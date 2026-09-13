# AvirA SDK Generator

Генератор C++ SDK для читов под Unity игры (Скорее больше подходит как Оффсет генератор, но да ладно). Берет дамп классов в виде `dump.cs`, который выдает наш дампер, и собирает из него готовые заголовки. Подключаешь их в свой софт и работаешь с классами игры как с обычными объектами: поля читаются и пишутся, методы вызываются, статика достается, енамы уже расписаны.

# AvirA SDK Generator

Генератор C++ SDK для читов под Unity игры. Берет дамп классов в виде `dump.cs` и собирает из него готовые заголовки с офсетами полей, дескрипторами методов и енамами. Никаких зависимостей у сгенеренного кода нет, только стандартная библиотека, поэтому его ест любое API: наш резолвер, чужой резолвер или свои сканеры с `call_function`.

## Какой дамп подойдет

Формат нашего дампера основной: головы классов с `TypeDefIndex`, секции полей и методов, офсеты в комментариях вида `// 0x`. Но парсер всеядный и понимает любой дамп в этом стиле, включая выхлоп Il2CppDumper. Главное чтобы в файле были строки имиджей, неймспейсы, классы с `TypeDefIndex`, поля с офсетами в `// 0x` комментариях и методы. Если у поля нет офсета, оно пропускается, остальное генерируется как обычно.

## Что нужно

Windows x64, MSVC v143, C++17. Больше ничего.

## Как пользоваться

Сначала делаешь дамп игры дампером, получаешь `dump.cs`. Потом запускаешь генератор:

```sh
AvirASdkGen.exe dump.cs sdk
```

Первый аргумент это дамп, второй куда сложить SDK. Второй можно не указывать, тогда офсеты падают в папку `Offsets` рядом с самим генератором. Дамп можно просто перетащить мышкой на exe, сработает так же.

На выходе лежат файлы: один `Sdk.hpp` общий и по одному `Sdk_<сборка>.hpp` на каждую сборку из дампа. Дальше подключаешь в чит:

```cpp
#include "Sdk.hpp"
```

После обновления игры просто снимаешь свежий дамп и перегенерируешь SDK, офсеты в сгенеренном коде привязаны к конкретной версии игры.

## Опции

```sh
AvirASdkGen.exe dump.cs sdk --image Assembly-CSharp --ns InventorySystem --skip Mirror
```

`--image` оставляет только сборки, в имени которых есть этот кусок. Можно несколько раз, тогда берутся все совпавшие. Без опции берутся все сборки.

`--ns` оставляет только классы, в неймспейсе которых есть этот кусок. Тоже можно несколько раз. Удобно когда нужен один кусок игры, а не все 5 тысяч классов.

`--skip` наоборот выкидывает классы, в полном имени которых есть этот кусок. Например `--skip Mirror` уберет сетевую библиотеку.

`--prefix` меняет префикс классов, по умолчанию `C_`. `--enumprefix` меняет префикс енамов, по умолчанию `E_`.

## Что генерируется

Каждый класс игры превращается в структуру вида `C_ReferenceHub` с константами. У структуры есть данные для поиска класса в рантайме: сборка, неймспейс, имя и вложенность. Дальше идут офсеты полей, статика отдельно в секции `Static`, у каждого метода дескриптор с именем и числом аргументов. Дескрипторы называются по методу в PascalCase без префиксов, геттеры и сеттеры схлопываются: `set_WantsToJump` дает `SetWantsToJump`:

```cpp
struct C_ReferenceHub
{
    static constexpr const char* ClassAssembly = "Assembly-CSharp.dll";
    static constexpr const char* ClassNamespace = "";
    static constexpr const char* ClassName = "ReferenceHub";
    static constexpr const char* ClassNested = "";
    struct Static
    {
        static constexpr unsigned long long _localHub = 0x38;
    };
    static constexpr unsigned long long characterClassManager = 0x88;
    struct GetHealth
    {
        static constexpr const char* Name = "get_Health";
        static constexpr int Args = 0;
    };
};
```

Енамы превращаются в `enum class` с теми же значениями что в игре:

```cpp
E_RoleType Role = E_RoleType::Scp173;
```

## Юзаж с нашим резолвером

Дескрипторы втыкаются в резолвер напрямую: класс находится по данным из структуры, метод по имени и числу аргументов, поле читается по офсету:

```cpp
#include "Sdk.hpp"

AvirA::C_Resolver Resolver;
Resolver.Initialize("GameAssembly.dll");

AvirA::C_Class HubClass = Resolver.ResolveClass(
    Global::C_ReferenceHub::ClassAssembly,
    Global::C_ReferenceHub::ClassNamespace,
    Global::C_ReferenceHub::ClassName);

using GetHealthFn = int(*)(AvirA::RawObject*);
GetHealthFn GetHealth = (GetHealthFn)HubClass
    .Method(Global::C_ReferenceHub::GetHealth::Name, Global::C_ReferenceHub::GetHealth::Args)
    .Pointer();

AvirA::C_Object Player(Resolver.Api(), PlayerPtr);
int Hp = Player.GetAt<int>(Global::C_PlayerStats::health);
```

## Юзаж с другим API

Смысл тот же, только резолв своим способом. Например с API в стиле Ливеруса дескриптор уходит в `Function` как есть:

```cpp
using GetHealthFn = int(*)(void*);
GetHealthFn GetHealth = Function<GetHealthFn>(
    Global::C_ReferenceHub::ClassAssembly, "",
    Global::C_ReferenceHub::ClassName,
    Global::C_PlayerStats::GetHealth::Name,
    Global::C_PlayerStats::GetHealth::Args);

int Hp = *(int*)((std::uint8_t*)PlayerPtr + Global::C_PlayerStats::health);
```

А если у тебя свои сканеры адресов и `call_function`, то из SDK нужны только офсеты полей и имена методов для скана, остальное живет как жило.

## Структура проекта

```
AvirASdkGen.sln      солюшен
AvirASdkGen.vcxproj  сам генератор, собирается в out/AvirASdkGen.exe
src/
  Log.hpp            маленький логгер
  Util.hpp           обрезка строк, разбор чисел, сплиты
  Dump.hpp/cpp       модель дампа и парсер dump.cs
  Types.hpp/cpp      санитайз имен и маппинг типов C# в C++
  Writer.hpp/cpp     генерация заголовков
  Main.cpp           аргументы командной строки и запуск
```

## Нюансы

Если у поля нет офсета в дампе, оно пропускается. Конструкторы не оборачиваются. Проперти отдельно не генерируются, потому что их геттеры и сеттеры уже есть в методах как `get_` и `set_`.

Поля с generic параметрами (типа `T` в `List<T>`) маппятся в `void*`, размер такого поля неизвестен на этапе генерации. Методы generic классов резолвятся по имени и числу аргументов как обычно.

Имена чистятся под C++: угловые скобки и мусор компилятора выкидываются, ключевые слова получают подчеркивание, дубли получают суффиксы `_2`, `_3`. Классы `<Module>` и `<PrivateImplementationDetails>` пропускаются, это шум.

## Лицензия

MIT, файл LICENSE в корне. Делай что хочешь, только сохрани копирайт.
