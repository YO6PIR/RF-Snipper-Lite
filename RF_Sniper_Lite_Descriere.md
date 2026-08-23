# RF Sniper Lite – Primul analizor RF din familia RF Sniper

RF Sniper Lite este primul aparat din seria RF Sniper, realizat ca un analizor RF compact construit în jurul unui ATmega8, al unui LCD HD44780 16×2 și al unui sintetizor Si5351.

## Hardware
- ATmega8, versiunea finală la 8 MHz
- LCD HD44780 16×2
- Si5351
- punte rezistivă de 50 Ω
- două intrări ADC pentru FWD și REV
- encoder rotativ
- un singur buton multifuncțional

## Micrografic RF cu 24 de puncte
Cele 8 caractere CGRAM ale HD44780 sunt folosite pentru a crea 24 de microbare independente: 8 caractere × 3 microbare = 24 puncte RF.

Graficul este actualizat progresiv, iar markerul poate fi deplasat pe toate cele 24 de poziții.

## Marker
Markerul este o linie verticală punctată XOR suprapusă direct peste microbara selectată. Encoderul mută markerul punct cu punct, iar afișajul indică frecvența și SWR-ul corespunzător.

## Scanare normală și panoramare
În modul SINGLE, după sweep, markerul se deplasează pe cele 24 de puncte. La margine, continuarea rotației encoderului produce edge-panning: graficul se deplasează cu un punct și se măsoară numai noua frecvență care intră în fereastră.

Domeniul RF este limitat la 1 MHz – 50 MHz.

## Continuous Scan
Prin apăsare lungă se intră în scanare continuă. Graficul este rescris progresiv fără ștergere completă, iar un cursor XOR funcționează ca scan-head. Encoderul poate deplasa centrul ferestrei în timpul scanării.

## Detectarea SWR minim
După fiecare sweep complet, firmware-ul determină automat SWR-ul minim și frecvența corespunzătoare. În Continuous Scan sunt afișate valoarea minimă și frecvența la care a fost găsită.

## STEP și SPAN
Versiunea actuală este adaptată micrograficului cu 24 de puncte. STEP reprezintă pasul în frecvență dintre microbare, iar SPAN-ul este derivat din geometria celor 24 de puncte.

## Interfață
Aparatul păstrează o interfață minimală: encoder, un singur buton și LCD 16×2, dar oferă scanare progresivă, marker, panoramare, Continuous Scan și detectarea automată a minimului.

## Resurse
Versiunea finală folosește practic întreaga memorie Flash a ATmega8, ajungând la aproximativ 99% utilizare, în timp ce SRAM-ul rămâne în limite confortabile.

RF Sniper Lite reprezintă practic limita funcțională a platformei ATmega8 pentru această aplicație.

## Filosofia proiectului
RF Sniper Lite nu încearcă să înlocuiască un analizor vectorial modern. Scopul lui este să ofere, cu hardware minimal, o reprezentare rapidă și intuitivă a comportamentului unei antene sau al unui circuit RF.

Din acest prim aparat au evoluat ulterior RF Sniper 2 și RF Sniper 3. RF Sniper Lite rămâne însă exemplul cel mai compact al ideii originale: un instrument RF foarte mic, construit cu resurse limitate, dar cu o experiență de utilizare surprinzător de completă.
