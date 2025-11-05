# PAK Extractor

Утилита для расшифровывания и извлечения содержимого архивов в формате `.pak`, используемых в играх **PopCap**.

Есть два режима:
* `-raw` извлекает содержимое не изменяя его.
* `-nice` извлекает содержимое объединяя и изменяя jp2,gif,ptx => прозрачный png.


Откуда взять файлы:
> **Важно:** проект не содержит файлов, созданных PopCap Games. Вы должны получить `.pak` самостоятельно из установленной игры.

Проверенно на следующих играх:
* Plants vs. Zombies **(PC)**
* Zuma's revenge **(PC)**
* Zuma's revenge **(PS3)**


## Установка зависимостей (vcpkg)
Замени `your-triplet` на свой собственный
```console
  vcpkg install stb openjpeg libsquish zlib giflib --triplet your-triplet
```

## Настройка параметров сборки
Открой `build.sh` и замени следующие переменные на свои:
```editor
triplet="your-triplet"
vcpkg_path="PATH/TO/VCPKG"
```

## Сборка
```bash
  bash build.sh
  cd build
  make
```


## Использование

Извлечь из одного файла:

```bash
./ExtractorPak File.pak -raw Out_RAW -nice Out_NICE
```

Извлечь из всех `.pak` в папке:

```bash
./ExtractorPak FolderWithPAKs -raw Out_RAW -nice Out_NICE
```

Флаги необязательны — можно указать только один:

```bash
./ExtractorPak File.pak -nice Out_NICE
```
