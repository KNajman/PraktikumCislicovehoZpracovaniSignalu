# Kodér a dekodér tónové volby (STM32 - Nucleo)

Tento semestrální projekt je implementací hardwarového kodéru a dekodéru tónové volby DTMF (Dual-Tone Multi-Frequency) na platformě STM32 Nucleo. Pro komunikaci a plynulý přesun dat jsem využil rozhraní SAI/I2S ve spolupráci s DMA, přičemž samotná detekce (dekódování) frekvencí probíhá pomocí implementace Goertzelova algoritmu.

**Autor:** Karel Najman (@KNajman)

## Přehled funkcionalit

- **Příjem audia:** Čtení 24bitového audia probíhá při vzorkovací frekvenci 8 kHz přes rozhraní I2S/SAI. Zpracování dat běží na pozadí pomocí DMA v režimu kruhového bufferu (Circular buffer).

- **Dekódování (Detekce) DTMF:** Bloky dat jsou analyzovány Goertzelovým algoritmem pro identifikaci standardních frekvencí. Systém obsahuje ochranu proti šumu a tzv. Twist Check pro kontrolu poměru amplitud. Zamezení duplicitní detekci téhož znaku (debounce) je nastaveno na 300 ms.

- **Kódování (Generování) tónů:** Program dokáže syntetizovat zachycené DTMF tóny a odesílat je jako zvukový výstup (echo) zpět do I2S/SAI.

- **Zobrazení dat:** Poslední čtyři dekódované znaky jsou asynchronně vypisovány na 7segmentovém displeji TM1637.

- **Sériová komunikace:** Souběžně s tím jsou dekódované znaky odesílány přes UART do PC (Baudrate: 115200, 8N1).

## Vlastní asynchronní ovladač pro TM1637

Pro obsluhu 7segmentového displeje jsem navrhl a implementoval vlastní neblokující (non-blocking) knihovnu TM1637_ASYNC.c.

Ačkoli byla funkčnost (sada příkazů pro displej) do jisté míry daná čipem TM1637, výchozí knihovna poskytnutá pro účely předmětu (doktorem Holadou) byla pro tuto aplikaci nepoužitelná, protože spoléhala na blokující zpoždění (delay). To narušovalo reálný čas zpracování audio vzorků a činilo systém nestabilním. Z toho důvodu jsem se rozhodl od základu změnit logiku fungování komunikace a celou knihovnu zcela přepsat.

Moje implementace řeší následující problémy:

- **Plně asynchronní chod:** Komunikace s displejem musí být asynchronní, protože z pohledu programu není předem známo, kdy přesně bude uživatel (nebo detekční logika) odesílat data k zobrazení. Využívám interní signálový buffer o velikosti 300 bajtů a přerušení hardwarového časovače pro řízené přepínání pinů (CLK/DIO). Zápis na displej tak nijak neblokuje hlavní smyčku ani výpočetní výkon potřebný pro zpracování audia.

- **Ochranu proti kolizím:** Nové požadavky na překreslení displeje jsou softwarově blokovány během již probíhajícího přenosu dat po sběrnici.

- **Podpora hardwarové rotace displeje:** Knihovna obsahuje softwarovou funkci, která dynamicky přepočítává mapování segmentů v případě, že je displej v zapojení fyzicky otočen o 180 stupňů.

## Zpracování signálu

Pro výpočetní operace a práci se signálem je využita standardní knihovna <arm_math.h>, která implementuje všechny potřebné DSP algoritmy. Proces digitálního zpracování signálu probíhá v následujících krocích:

1. DMA kontinuálně plní polovinu bufferu rx_buffer (velikost bloku činí 2048 vzorků). Přenos je efektivně řízen pomocí událostí (callbacků) přes standardní knihovnu HAL.
2. Přijatá 24bitová data jsou softwarově normalizována a předávána Goertzelovu algoritmu, který vypočítá energii (magnitudu) pro 4 nízké a 4 vysoké DTMF frekvence.
3. Vyhodnocovací logika zkontroluje, zda nejsilnější frekvence překračují experimentálně stanovený práh (DTMF_THRESHOLD = 20.0f). Následně proběhne tzv. Twist Check (ověřující poměr obou frekvencí v toleranci 0.25 až 4.0). Tato validace je nezbytná, protože systém využívá USB s audio vstupy a do přenosového kanálu se občas zanáší šum, pískání a jiné nežádoucí artefakty, které by mohly měnit přenášenou frekvenci.
4. Pokud je dekódován platný znak, program aplikuje časový zámek proti vícenásobné detekci. Následně je znak vypsán na UART, je vyvolána aktualizace displeje TM1637 a zahájí se vysílání zakódovaného echa zpět do audia.

## Možnosti vylepšení

V současné verzi se pro čištění signálu nepoužívá žádný aktivní filtr, je aplikováno pouze jednoduché prahování (thresholding). Skvělou možností pro budoucí zlepšení přesnosti detekce by bylo signál nejprve vyfiltrovat (např. pomocí pásmové propusti) a až poté jej předávat ke zpracování Goertzelovým algoritmem.

## Ovládání a interakce

- Po inicializaci odešle mikrokontrolér uvítací zprávu "Nucleo START\r\n" přes UART.
- Při úspěšném dekódování tónu systém automaticky reaguje výpisem do terminálu a okamžitým zobrazením na displeji.
- Modré uživatelské tlačítko (B1) na vývojové desce je nastaveno jako přerušení (EXTI) a slouží k manuálnímu vymazání paměti displeje.

## Živá ukázka

[![Ukázka detekce DTMF](ziva_ukazka.mp4)](ziva_ukazka.mp4)
