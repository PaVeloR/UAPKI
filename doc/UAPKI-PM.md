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

#### Нарізка файлів

Якщо використовуються `fileBlockOffset` та `fileBlockLength`:

- Обробляється лише вказаний діапазон файлів
- Вихід за межі діапазону НЕ є помилкою
- Може обробити 0 байтів
- За замовчуванням:
- offset = 0
- length = UINT64_MAX (весь файл)

---

#### Помилки

| Code | Name |
|------|------|
| 4128 | DIGEST_CONTEXT_NOT_INITIALIZED |

---

## Приклади

### 1. Звичайне хешування файла

Це контрольний приклад, від якого всі відштовхуються.

```json
{
  "method": "DIGEST",
  "parameters": {
    "hashAlgo": "2.16.840.1.101.3.4.2.1",
    "file": "R:/la.puf"
  }
}
```

Результат:

- хешується весь файл
- повертається bytes

### 2. Початок покрокового хешування (DoUpdate=false, DoFinalize=false)

Це перший важливий приклад.

```json
{
  "method": "DIGEST",
  "parameters": {
    "hashAlgo": "2.16.840.1.101.3.4.2.1",
    "file": "R:/la.puf",
    "fileBlockOffset": 0,
    "fileBlockLength": 100,
    "DoUpdate": false,
    "DoFinalize": false
  }
}
```

#### Що відбувається:

- попередній контекст, якщо був, знищується
- створюється новий контекст
- у нього додаються перші 100 байт файла
- bytes у result не повертається

### 3. Продовження покрокового хешування (DoUpdate=true)

Оце один із головних прикладів.

```json
{
  "method": "DIGEST",
  "parameters": {
    "file": "R:/la.puf",
    "fileBlockOffset": 100,
    "fileBlockLength": 200,
    "DoUpdate": true,
    "DoFinalize": false
  }
}
```

#### Що відбувається:

- використовується вже ініціалізований контекст
- до нього додаються байти файла з діапазону [100, 300]
- hashAlgo вказувати не можна
- bytes у result не повертається


### 4. ByteRange-подібний сценарій

Оце якраз те, що треба показати, навіть якщо не згадувати PDF як формат.

Крок 1

```json
{
  "method": "DIGEST",
  "parameters": {
    "hashAlgo": "2.16.840.1.101.3.4.2.1",
    "file": "R:/signed.pdf",
    "fileBlockOffset": 0,
    "fileBlockLength": 12345,
    "DoUpdate": false,
    "DoFinalize": false
  }
}
```
Крок 2
```json
{
  "method": "DIGEST",
  "parameters": {
    "file": "R:/signed.pdf",
    "fileBlockOffset": 45678,
    "fileBlockLength": 9876,
    "DoUpdate": true,
    "DoFinalize": false
  }
}
```
Крок 3
```json
{
  "method": "DIGEST",
  "parameters": {
    "DoUpdate": true,
    "DoFinalize": true
  }
}
```
#### Пояснення:

хеш обчислюється не для всього файла, а для двох окремих діапазонів
це дозволяє обробляти файл частинами без попереднього копіювання цих частин у пам’ять
на останньому кроці нові дані можна не передавати, лише завершити контекст.

### 5. Поведінка при виході за межі файла

```json
{
  "method": "DIGEST",
  "parameters": {
    "hashAlgo": "2.16.840.1.101.3.4.2.1",
    "file": "R:/la.puf",
    "fileBlockOffset": 999999999,
    "fileBlockLength": 100
  }
}
```

#### Пояснення:

помилка не виникає
якщо після fileBlockOffset у файлі нема даних, обробляється 0 байт.
Команда визначає діапазон і хеш обчислюється тіки з тих байтів що в нього влучили