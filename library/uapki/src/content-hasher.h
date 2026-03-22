#ifndef UAPKI_CONTENT_HASHER_H
#define UAPKI_CONTENT_HASHER_H


#include "uapki-ns.h"
#include "hash.h"
#include <string>


namespace UapkiNS {


class ContentHasher {
public:
    enum class SourceType : uint32_t {
        UNDEFINED   = 0,
        BYTEARRAY   = 1,
        FILE        = 2,
        MEMORY      = 3
    };  //  end enum SourceType

private:
    SourceType  m_SourceType;
    HashAlg     m_HashAlgo;
    HashCtx*    m_HashCtx;
    ByteArray*  m_Bytes;
    bool        m_AutoReleaseBytes;
    std::string m_Filename;
    const uint8_t*
                m_MemoryPtr;
    size_t      m_MemorySize;
    SmartBA     m_Value;

public:
    ContentHasher (void);
    ~ContentHasher (void);

    int digest (
        const HashAlg hashAlgo
    );

    int Start (
        const HashAlg hashAlgo
    );
    int Update (void);
    int Finalize (void);

    void reset (void);

    int setContent (
        const ByteArray* baBytes,
        const bool autoRelease
    );
    int setContent (
        const char* filename
    );
    int setContent (
        const uint8_t* ptr,
        const size_t size
    );

public:
    const ByteArray* getContentBytes (void) const {
        return m_Bytes;
    }
    const ByteArray* getHashValue (void) const {
        return m_Value.get();
    }
    SourceType getSourceType (void) const {
        return m_SourceType;
    }
    bool isPresent (void) const {
        return (m_SourceType != SourceType::UNDEFINED);
    }
    bool isInitialized (void) const {
        return (m_HashCtx != nullptr);
    }

public:
    static const uint8_t* baToPtr (
        const ByteArray* baPtr
    );
    static bool numberToSize (
        const double fSize,
        size_t& size
    );

private:
    int digestFile (
        const HashAlg hashAlgo
    );
    int digestMemory (
        const HashAlg hashAlgo
    );

    int updateCtx (
        HashCtx* hashCtx
    );
    int updateFile (
        HashCtx* hashCtx
    );
    int updateMemory (
        HashCtx* hashCtx
    );

    void resetContent (void);
    void resetHashCtx (void);
    void setSourceType (
        const SourceType sourceType
    );

};  //  end class ContentHasher


}   //  end namespace UapkiNS


#endif