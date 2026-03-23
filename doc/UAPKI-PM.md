# UAPKI Programming Manual

## Version

- Library: UAPKI
- Version: 2.0.12-pavelor.1

---

## DIGEST

### Description

Calculates hash of provided content.

---

### Parameters

| Name              | Type    | Required | Default        | Description |
|-------------------|---------|----------|----------------|------------|
| hashAlgo          | string  | yes      | —              | Hash algorithm OID |
| bytes             | base64  | no       | —              | Data buffer |
| file              | string  | no       | —              | File path |
| ptr + size        | hex+num | no       | —              | Memory pointer |
| DoUpdate          | bool    | no       | false          | Continue existing digest |
| DoFinalize        | bool    | no       | true           | Finalize digest |
| fileBlockOffset   | uint64  | no       | 0              | File offset |
| fileBlockLength   | uint64  | no       | UINT64_MAX     | Length of file block |

---

### Behavior

#### One-shot (default)

DoUpdate = false
DoFinalize = true


- Computes hash immediately
- Returns `bytes`

---

#### Start incremental

DoUpdate = false
DoFinalize = false


- Starts new digest context
- No result returned

---

#### Update

DoUpdate = true
DoFinalize = false


- Adds data to existing digest
- Requires initialized context

---

#### Finalize

DoUpdate = true
DoFinalize = true


- Finalizes digest
- Returns `bytes`

---

### File slicing

If `fileBlockOffset` and `fileBlockLength` are used:

- Only specified file range is processed
- Out-of-range is NOT an error
- May process 0 bytes
- Default:
  - offset = 0
  - length = UINT64_MAX (whole file)

---

### Errors

| Code | Name |
|------|------|
| 4128 | DIGEST_CONTEXT_NOT_INITIALIZED |

---

### Example

```json
{
  "method": "DIGEST",
  "parameters": {
    "hashAlgo": "2.16.840.1.101.3.4.2.1",
    "file": "test.bin"
  }
}