/*
 * Copyright (c) 2021, The UAPKI Project Authors.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define FILE_MARKER "uapki/api/digest.cpp"

#include "api-json-internal.h"
#include "content-hasher.h"
#include "oid-utils.h"
#include "parson-helper.h"


#define DEBUG_OUTCON(expression)
#ifndef DEBUG_OUTCON
#define DEBUG_OUTCON(expression) expression
#endif


using namespace std;
using namespace UapkiNS;


static ContentHasher g_content_hasher;


static bool json_get_bool_param (
        JSON_Object* joParams,
        const char* name,
        bool& value,
        bool& isPresent
)
{
    isPresent = false;
    value = false;

    if (!json_object_has_value(joParams, name)) {
        return true;
    }

    isPresent = true;
    if (!ParsonHelper::jsonObjectHasValue(joParams, name, JSONBoolean)) {
        return false;
    }

    value = json_object_get_boolean(joParams, name) ? true : false;
    return true;
}

static bool has_content_params (
        JSON_Object* joParams
)
{
    return (
        ParsonHelper::jsonObjectHasValue(joParams, "bytes", JSONString) ||
        ParsonHelper::jsonObjectHasValue(joParams, "file", JSONString) ||
        (
            ParsonHelper::jsonObjectHasValue(joParams, "ptr", JSONString) &&
            ParsonHelper::jsonObjectHasValue(joParams, "size", JSONNumber)
        )
    );
}

static int set_content_from_params (
        ContentHasher& contentHasher,
        JSON_Object* joParams
)
{
    if (ParsonHelper::jsonObjectHasValue(joParams, "bytes", JSONString)) {
        return contentHasher.setContent(json_object_get_base64(joParams, "bytes"), true);
    }

    if (ParsonHelper::jsonObjectHasValue(joParams, "file", JSONString)) {
        return contentHasher.setContent(json_object_get_string(joParams, "file"));
    }

    if (
        ParsonHelper::jsonObjectHasValue(joParams, "ptr", JSONString) &&
        ParsonHelper::jsonObjectHasValue(joParams, "size", JSONNumber)
    ) {
        SmartBA sba_ptr;
        (void)sba_ptr.set(json_object_get_hex(joParams, "ptr"));

        const uint8_t* ptr = ContentHasher::baToPtr(sba_ptr.get());
        size_t size = 0;
        if (!ptr || !ContentHasher::numberToSize(json_object_get_number(joParams, "size"), size)) {
            return RET_UAPKI_INVALID_PARAMETER;
        }

        return contentHasher.setContent(ptr, size);
    }

    return RET_UAPKI_INVALID_PARAMETER;
}


int uapki_digest (JSON_Object* joParams, JSON_Object* joResult)
{
    int ret = RET_OK;
    bool do_update = false;
    bool do_finalize = true;
    bool is_present = false;
    bool has_content = false;

    const char* s_hashalgo = nullptr;
    const char* s_signalgo = nullptr;
    HashAlg hash_algo = HashAlg::HASH_ALG_UNDEFINED;

    // DoUpdate
    if (!json_get_bool_param(joParams, "DoUpdate", do_update, is_present)) {
        SET_ERROR(RET_UAPKI_INVALID_PARAMETER);
    }

    // DoFinalize
    if (!json_get_bool_param(joParams, "DoFinalize", do_finalize, is_present)) {
        SET_ERROR(RET_UAPKI_INVALID_PARAMETER);
    }

    // Continuation syntax: algorithm is forbidden
    if (do_update) {
        if (json_object_has_value(joParams, "hashAlgo") || json_object_has_value(joParams, "signAlgo")) {
            SET_ERROR(RET_UAPKI_INVALID_PARAMETER);
        }
    }
    else {
        s_hashalgo = json_object_get_string(joParams, "hashAlgo");
        if (!s_hashalgo) {
            s_signalgo = json_object_get_string(joParams, "signAlgo");
        }
        if (!s_hashalgo && !s_signalgo) {
            SET_ERROR(RET_UAPKI_INVALID_PARAMETER);
        }

        hash_algo = hash_from_oid((s_hashalgo) ? s_hashalgo : s_signalgo);
        if (hash_algo == HashAlg::HASH_ALG_UNDEFINED) {
            SET_ERROR(RET_UAPKI_UNSUPPORTED_ALG);
        }

        DO_JSON(json_object_set_string(
            joResult,
            "hashAlgo",
            (s_hashalgo) ? s_hashalgo : hash_to_oid(hash_algo)
        ));
    }

    has_content = has_content_params(joParams);

    // ===== Old one-shot logic =====
    if (!do_update && do_finalize) {
        ContentHasher content_hasher;

        // Per agreed semantics: destroy previous active state.
        g_content_hasher.reset();

        DO(set_content_from_params(content_hasher, joParams));
        DO(content_hasher.digest(hash_algo));

        DO_JSON(json_object_set_base64(joResult, "bytes", content_hasher.getHashValue()));
        goto cleanup;
    }

    // ===== Start new active digest =====
    if (!do_update && !do_finalize) {
        g_content_hasher.reset();

        DO(set_content_from_params(g_content_hasher, joParams));
        DO(g_content_hasher.Start(hash_algo));

        goto cleanup;
    }

    // ===== Continuation =====
    if (!g_content_hasher.isInitialized()) {
        SET_ERROR(RET_UAPKI_DIGEST_CONTEXT_NOT_INITIALIZED);
    }

    if (has_content) {
        DO(set_content_from_params(g_content_hasher, joParams));
        DO(g_content_hasher.Update());
    }

    if (do_finalize) {
        DO(g_content_hasher.Finalize());
        DO_JSON(json_object_set_base64(joResult, "bytes", g_content_hasher.getHashValue()));
    }

cleanup:
    return ret;
}