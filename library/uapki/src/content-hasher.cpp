#define FILE_MARKER "uapki/content-hasher.cpp"

#include "content-hasher.h"
#include "ba-utils.h"
#include "macros-internal.h"
#include "uapkic-errors.h"
#include "uapki-errors.h"
#include <cstring>


#define FILE_BLOCK_SIZE (10 * 1024 * 1024)


 //  See: byte-array-internal.h
struct ByteArray_st {
    const uint8_t*  buf;
    size_t          len;
};


using namespace std;

namespace UapkiNS {


ContentHasher::ContentHasher (void)
    : m_SourceType(SourceType::UNDEFINED)
    , m_HashAlgo(HASH_ALG_UNDEFINED)
    , m_HashCtx(nullptr)
    , m_Bytes(nullptr)
    , m_AutoReleaseBytes(false)
    , m_MemoryPtr(nullptr)
    , m_MemorySize(0)
{
}

ContentHasher::~ContentHasher (void)
{
    reset();
}

int ContentHasher::digest (
        const HashAlg hashAlgo
)
{
    if (
        (hashAlgo == HASH_ALG_UNDEFINED) ||
        (m_SourceType == SourceType::UNDEFINED)
    ) RET_UAPKI_INVALID_PARAMETER;

    if (m_HashAlgo == hashAlgo) return RET_OK;

    setSourceType(m_SourceType);

    int ret = RET_OK;
    switch (m_SourceType) {
    case SourceType::BYTEARRAY:
        ret = ::hash(hashAlgo, m_Bytes, &m_Value);
        break;
    case SourceType::FILE:
        ret = digestFile(hashAlgo);
        break;
    case SourceType::MEMORY:
        ret = digestMemory(hashAlgo);
        break;
    default:
        ret = RET_UAPKI_INVALID_PARAMETER;
        break;
    }

    if (ret == RET_OK) {
        m_HashAlgo = hashAlgo;
    }
    return ret;
}

int ContentHasher::Start (
        const HashAlg hashAlgo
)
{
    if (hashAlgo == HASH_ALG_UNDEFINED) {
        return RET_UAPKI_INVALID_PARAMETER;
    }

    resetHashCtx();
    m_Value.clear();

    m_HashCtx = hash_alloc(hashAlgo);
    if (!m_HashCtx) {
        m_HashAlgo = HASH_ALG_UNDEFINED;
        return RET_UAPKI_GENERAL_ERROR;
    }

    m_HashAlgo = hashAlgo;

    const int ret = updateCtx(m_HashCtx);
    if (ret != RET_OK) {
        resetHashCtx();
        m_HashAlgo = HASH_ALG_UNDEFINED;
        return ret;
    }

    resetContent();
    return RET_OK;
}

int ContentHasher::Update (void)
{
    if (!m_HashCtx) {
        return RET_UAPKI_DIGEST_CONTEXT_NOT_INITIALIZED;
    }

    const int ret = updateCtx(m_HashCtx);
    if (ret != RET_OK) {
        resetHashCtx();
        m_HashAlgo = HASH_ALG_UNDEFINED;
        return ret;
    }

    resetContent();
    return RET_OK;
}

int ContentHasher::Finalize (void)
{
    if (!m_HashCtx) {
        return RET_UAPKI_DIGEST_CONTEXT_NOT_INITIALIZED;
    }

    int ret = hash_final(m_HashCtx, &m_Value);
    resetHashCtx();
    if (ret != RET_OK) {
        m_HashAlgo = HASH_ALG_UNDEFINED;
        return ret;
    }

    return RET_OK;
}

void ContentHasher::reset (void)
{
    resetContent();
    resetHashCtx();
    m_HashAlgo = HASH_ALG_UNDEFINED;
    m_Value.clear();
}

int ContentHasher::setContent (
        const ByteArray* baBytes,
        const bool autoRelease
)
{
    if (!baBytes) return RET_UAPKI_INVALID_PARAMETER;

    setSourceType(SourceType::BYTEARRAY);
    m_Bytes = (ByteArray*)baBytes;
    m_AutoReleaseBytes = autoRelease;
    return RET_OK;
}

int ContentHasher::setContent (
        const char* filename
)
{
    if (!filename) return RET_UAPKI_INVALID_PARAMETER;

    setSourceType(SourceType::FILE);
    m_Filename = string(filename);
    return RET_OK;
}

int ContentHasher::setContent (
        const uint8_t* ptr,
        const size_t size
)
{
    if (!ptr) return RET_UAPKI_INVALID_PARAMETER;

    setSourceType(SourceType::MEMORY);
    m_MemoryPtr = ptr;
    m_MemorySize = size;
    return RET_OK;
}

const uint8_t* ContentHasher::baToPtr (
        const ByteArray* baPtr
)
{
    const uint8_t* rv_ptr = nullptr;
    if ((ba_get_len(baPtr) == sizeof(void*))) {
        (void)ba_swap(baPtr);
        memcpy(&rv_ptr, ba_get_buf_const(baPtr), sizeof(void*));
        (void)ba_swap(baPtr);
    }
    return rv_ptr;
}

bool ContentHasher::numberToSize (
        const double fSize,
        size_t& size
)
{
    size = (size_t)fSize;
    return (fSize >= 0) && ((double)size == fSize);
}

int ContentHasher::digestFile (
        const HashAlg hashAlgo
)
{
    int ret = RET_OK;
    HashCtx* hash_ctx = nullptr;
    ByteArray* ba_data = nullptr;
    FILE* f = nullptr;

    f = fopen_utf8(m_Filename.c_str(), 0);
    if (!f) {
        SET_ERROR(RET_UAPKI_FILE_OPEN_ERROR);
    }

    CHECK_NOT_NULL(hash_ctx = hash_alloc(hashAlgo));

    CHECK_NOT_NULL(ba_data = ba_alloc_by_len(FILE_BLOCK_SIZE));

    do {
        ba_data->len = fread(ba_get_buf(ba_data), 1, FILE_BLOCK_SIZE, f);
        DO(hash_update(hash_ctx, ba_data));
    } while (ba_data->len == FILE_BLOCK_SIZE);

    if (ferror(f)) {
        SET_ERROR(RET_UAPKI_FILE_READ_ERROR);
    }

    DO(hash_final(hash_ctx, &m_Value));

cleanup:
    if (f) {
        fclose(f);
    }
    ba_free(ba_data);
    hash_free(hash_ctx);
    return ret;
}

int ContentHasher::digestMemory (
        const HashAlg hashAlgo
)
{
    ByteArray ba_local = { m_MemoryPtr, m_MemorySize };
    return ::hash(hashAlgo, &ba_local, &m_Value);
}

int ContentHasher::updateCtx (
        HashCtx* hashCtx
)
{
    if (!hashCtx) {
        return RET_UAPKI_INVALID_PARAMETER;
    }

    switch (m_SourceType) {
    case SourceType::UNDEFINED:
        return RET_OK;

    case SourceType::BYTEARRAY:
        return hash_update(hashCtx, m_Bytes);

    case SourceType::FILE:
        return updateFile(hashCtx);

    case SourceType::MEMORY:
        return updateMemory(hashCtx);

    default:
        return RET_UAPKI_INVALID_PARAMETER;
    }
}

int ContentHasher::updateFile (
        HashCtx* hashCtx
)
{
    int ret = RET_OK;
    ByteArray* ba_data = nullptr;
    FILE* f = nullptr;

    f = fopen_utf8(m_Filename.c_str(), 0);
    if (!f) {
        SET_ERROR(RET_UAPKI_FILE_OPEN_ERROR);
    }

    CHECK_NOT_NULL(ba_data = ba_alloc_by_len(FILE_BLOCK_SIZE));

    do {
        ba_data->len = fread(ba_get_buf(ba_data), 1, FILE_BLOCK_SIZE, f);
        DO(hash_update(hashCtx, ba_data));
    } while (ba_data->len == FILE_BLOCK_SIZE);

    if (ferror(f)) {
        SET_ERROR(RET_UAPKI_FILE_READ_ERROR);
    }

cleanup:
    if (f) {
        fclose(f);
    }
    ba_free(ba_data);
    return ret;
}

int ContentHasher::updateMemory (
        HashCtx* hashCtx
)
{
    ByteArray ba_local = { m_MemoryPtr, m_MemorySize };
    return hash_update(hashCtx, &ba_local);
}

void ContentHasher::resetContent (void)
{
    if (m_AutoReleaseBytes && m_Bytes) {
        ba_free(m_Bytes);
    }

    m_Bytes = nullptr;
    m_AutoReleaseBytes = false;
    m_Filename.clear();
    m_MemoryPtr = nullptr;
    m_MemorySize = 0;
    m_SourceType = SourceType::UNDEFINED;
}

void ContentHasher::resetHashCtx (void)
{
    if (m_HashCtx) {
        hash_free(m_HashCtx);
        m_HashCtx = nullptr;
    }
}

void ContentHasher::setSourceType (
        const SourceType sourceType
)
{
    resetContent();
    m_SourceType = sourceType;
    m_HashAlgo = HASH_ALG_UNDEFINED;
    m_Value.clear();
}


}   //  end namespace UapkiNS