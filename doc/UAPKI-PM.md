# UAPKI Programming Manual

## Version

- Library: UAPKI
- Version: 2.0.12-pavelor.1

---

## DIGEST

### Description

Обчислює хеш наданого контенту.

---

### Parameters

| Параметр          |          Тип | Обов’язковий | За замовчуванням | Опис                                              |
| ----------------- | -----------: | :----------: | ---------------: | ------------------------------------------------- |
| `hashAlgo`        |       string |     так*     |                — | OID алгоритму хешування                           |
| `bytes`           |       base64 |      ні      |                — | Дані у вигляді base64                             |
| `file`            |       string |      ні      |                — | Шлях до файла                                     |
| `ptr` + `size`    | hex + number |      ні      |                — | Вказівник на пам’ять і розмір                     |
| `DoUpdate`        |         bool |      ні      |          `false` | Продовжити вже ініціалізований контекст хешування |
| `DoFinalize`      |         bool |      ні      |           `true` | Завершити обчислення хешу                         |
| `fileBlockOffset` |       uint64 |      ні      |              `0` | Зміщення від початку файла                        |
| `fileBlockLength` |       uint64 |      ні      |     `UINT64_MAX` | Довжина блока файла                               |

---

### Поведінка

#### One-shot (за замовчуванням)

DoUpdate = false
DoFinalize = true


- Обчислює хеш негайно
- Повертає `байти`

---

#### Start incremental

DoUpdate = false
DoFinalize = false


- Запускає новий контекст дайджесту
- Результатів не повернуто

---

#### Update

DoUpdate = true
DoFinalize = false


- Додає дані до існуючого дайджесту
- Потрібен ініціалізований контекст

---

#### Finalize

DoUpdate = true
DoFinalize = true


- Завершує дайджест
- Повертає `байти`

---

### Нарізка файлів

Якщо використовуються `fileBlockOffset` та `fileBlockLength`:

- Обробляється лише вказаний діапазон файлів
- Вихід за межі діапазону НЕ є помилкою
- Може обробити 0 байтів
- За замовчуванням:
- offset = 0
- length = UINT64_MAX (весь файл)

---

### Помилки

| Code | Name |
|------|------|
| 4128 | DIGEST_CONTEXT_NOT_INITIALIZED |

---

### Приклади

```json
{
  "method": "DIGEST",
  "parameters": {
    "file": "С:/xx.pdf",
    "fileBlockOffset": 100,
    "fileBlockLength": 200,
    "DoUpdate": true,
    "DoFinalize": false
  }
}
```

### Що відбувається:

- використовується вже ініціалізований контекст
- до нього додаються байти файла з діапазону [100, 300]
- hashAlgo вказувати не можна
- bytes у result не повертається (бо вказано що це ше не фінал 
Увага:
При DoUpdate=true параметри hashAlgo і signAlgo заборонені.