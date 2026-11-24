# PDF Merge GS

Desktop alat za spajanje PDF dokumenata na Linuxu (Qt 6, KDE Plasma).
Spajanje obavlja vanjski `gs` (Ghostscript), pa aplikacija nudi odabir
profila kvalitete umjesto vlastite implementacije PDF pisanja.

## Značajke

- Dodavanje PDF-ova iz Dolphina (drag & drop) ili dijalogom, uz pamćenje
  zadnje korištene mape.
- Sličica prve stranice za svaki dokument, s nazivom i brojem stranica.
- Promjena redoslijeda povlačenjem, brisanje tipkom Delete, kontekstnim
  izbornikom ili alatnom trakom.
- Detekcija digitalnog potpisa i upozorenje prije spajanja: potpisi se
  spajanjem trajno uklanjaju.
- Asinkrono spajanje (`QProcess`) s busy indikatorom i mogućnošću prekida.
- Sučelje na engleskom i hrvatskom, s automatskim prepoznavanjem jezika
  sustava i ručnim odabirom u postavkama.
- Pokretanje iz naredbenog retka i iz Dolphina (`Otvori pomoću`).

## Ovisnosti

| Distribucija | Paketi |
|---|---|
| openSUSE Leap / Tumbleweed | `cmake gcc-c++ qt6-qtbase-devel qt6-qtpdf-devel qt6-qttools-devel ghostscript` |
| Fedora | `cmake gcc-c++ qt6-qtbase-devel qt6-qtpdf-devel qt6-qttools-devel ghostscript` |
| Debian / Ubuntu | `cmake g++ qt6-base-dev qt6-pdf-dev qt6-tools-dev ghostscript` |

`ghostscript` mora biti prisutan u vrijeme rada, ne samo u vrijeme gradnje.
Ako ga nema, aplikacija prikaže traku upozorenja, dopušta slaganje
dokumenata, ali onemogući gumb za spajanje.

## Gradnja

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
```

U `build/` se uz izvršnu datoteku generiraju i `pdfmergegs.desktop` te
`pdfmergegs.svg`, pa se aplikacija može pokrenuti i bez instalacije:

```bash
./build/pdfmergegs
```

Instalacija:

```bash
sudo cmake --install build
```

Time završe u `${CMAKE_INSTALL_BINDIR}`, `${CMAKE_INSTALL_DATADIR}/applications`
i `${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps`.

Napomena: `Exec=` u `.desktop` datoteci sadrži punu putanju do binarne
datoteke i upisuje se u vrijeme konfiguracije. Ako mijenjaš instalacijski
prefix, ponovno pokreni `cmake` s novim `-DCMAKE_INSTALL_PREFIX`.

## Prijevodi

Svi stringovi u kodu su na engleskom i omotani u `tr()`. Hrvatski prijevod
je u `translations/pdfmergegs_hr.ts` i ugrađuje se u binarnu datoteku kao
Qt resurs (`:/i18n/pdfmergegs_hr.qm`), pa nema vanjskih `.qm` datoteka.

- Dopuna prijevoda nakon promjene stringova: `cmake --build build --target update_translations`
- Ručno pečenje `.qm`: `lrelease translations/pdfmergegs_hr.ts -qm pdfmergegs_hr.qm`

Na Qt 6.5 i novijem `.ts` → `.qm` i ugrađivanje radi `qt_add_translations`.
Na starijim verzijama (npr. 6.4 na Leapu 15.6) CMake to radi u dva koraka:
`lrelease` pa `qt_add_resources`.

## Korištenje

```bash
pdfmergegs                       # prazna aplikacija
pdfmergegs prvi.pdf drugi.pdf    # datoteke se učitaju pri pokretanju
pdfmergegs --help
```

U Dolphinu: desni klik na PDF → `Otvori pomoću` → `PDF Merge GS`, ili
odabir više datoteka pa isto. `.desktop` datoteka koristi `%F`, pa se
prosljeđuju sve označene datoteke.

## Postavke

Sve postavke su u `~/.config/PdfMergeGs/config.ini` (`QSettings`, INI):

| Ključ | Značenje |
|---|---|
| `ui/language` | `0` zadano sustava, `1` engleski, `2` hrvatski |
| `ghostscript/binary` | vlastita putanja do `gs` (prazno = `PATH`) |
| `ghostscript/preset` | `0` bez gubitaka, `1` ispis, `2` e-pošta, `3` prilagođeno |
| `ghostscript/arguments` | lista argumenata koja se predaje Ghostscriptu |
| `paths/lastSourceDir` | zadnja mapa iz koje su dodani PDF-ovi |
| `paths/lastDestinationDir` | zadnja mapa u koju je spremljen rezultat |

`Restore Defaults` u dijalogu briše cijelu datoteku i vraća zadane
vrijednosti (mape se vraćaju na `Documents`).

## Profili kvalitete

1. **Bez gubitaka (preporučeno)** — `-dPDFSETTINGS=/default` uz isključeno
   downsampleanje slika i `-dColorConversionStrategy=/LeaveColorUnchanged`.
2. **Visoka kvaliteta / ispis** — `-dPDFSETTINGS=/printer` (300 dpi).
3. **Komprimirano / e-pošta** — `-dPDFSETTINGS=/ebook` (150 dpi).
4. **Prilagođeno** — slobodan unos argumenata.

`-sOutputFile`, `-f` i popis ulaznih datoteka dodaju se automatski, pa ih
u prilagođenim argumentima ne treba navoditi.

## RPM pakiranje

```bash
# iz korijena projekta
tar czf pdfmergegs-1.0.0.tar.gz --exclude=build --exclude='*.rpm' .
rpmbuild -ta pdfmergegs-1.0.0.tar.gz
```

Spec datoteka je u `deploy/rpm/pdfmergegs.spec` i koristi `%cmake`,
`%cmake_build` i `%cmake_install`. Deklarira `Requires: ghostscript`.

## Struktura

```
CMakeLists.txt
src/
  main.cpp             ulazna točka, QCommandLineParser, jezik
  MainWindow.*         alatna traka, lista, tijek spajanja
  PdfListWidget.*      ikone, drag & drop, redoslijed, brisanje
  PdfDocumentInfo.*    broj stranica, detekcija potpisa, sličice
  GhostscriptRunner.*  asinkroni QProcess
  SettingsDialog.*     jezik, gs putanja, profili, argumenti
  AppSettings.*        QSettings wrapper (config.ini)
  LanguageManager.*    učitavanje prijevoda iz resursa
translations/
  pdfmergegs_hr.ts
deploy/
  pdfmergegs.svg
  pdfmergegs.desktop.in
  rpm/pdfmergegs.spec
```

## Licenca

GPL-3.0-or-later, vidi `LICENSE`.
