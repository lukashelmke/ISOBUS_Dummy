# ISOBUS Dummy mit ESP32

Dieses Projekt ist eine Weiterentwicklung des [**ISOBUS_VT_Client_ESP32_Repo**](https://github.com/lukashelmke/IsoBus_VT_Client_ESP32_Repo/tree/main).

Ziel dieses Repositories ist die Entwicklung eines **kompakten ISOBUS-Dummys**, der eine **reale ISOBUS-Maschine funktional nachbildet** und als **Ersatzsystem** für Tests, Entwicklung und Demonstration dient.

Der Dummy wird direkt über einen ISOBUS-Stecker an ein ISOBUS-fähiges Gerät (z. B. Terminal, ECU, Nachrüstsystem) angeschlossen und verhält sich aus Sicht des Kommunikationspartners **wie eine echte Maschine**.

---

## Projektidee & Motivation

ISOBUS- und CAN-basierte Systeme lassen sich in der Praxis oft nur eingeschränkt testen, da:
- eine reale Maschine nicht immer verfügbar ist
- Tests im Feld zeitaufwendig und kostenintensiv sind
- Entwicklungs- und Debug-Zugriffe an produktiven Maschinen unerwünscht sind

Der **ISOBUS Dummy** ermöglicht es, **ISOBUS-Systeme im Labor oder in der Werkstatt** zu entwickeln, zu testen und zu demonstrieren, **ohne eine reale Maschine zu benötigen**.

Typische Einsatzszenarien:
- Testen von **ISOBUS-VT-Clients**
- Entwicklung und Prüfung von **CAN-BUS / ISOBUS-Nachrüstlösungen**
- Funktionstests von Terminals, Steuergeräten oder Gateways
- Schulung, Demonstration und Fehlersuche

Der Dummy übernimmt dabei die Rolle der Maschine und stellt die notwendigen ISOBUS-Funktionen und Schnittstellen bereit.

---

## Abgrenzung zum ursprünglichen Projekt

Während sich das  
[**ISOBUS_VT_Client_ESP32_Repo**](https://github.com/lukashelmke/IsoBus_VT_Client_ESP32_Repo/tree/main) primär auf die **Software eines VT-Clients** konzentriert, erweitert dieses Projekt den Ansatz um eine **vollständig eigenentwickelte Hardware-Plattform**.

Der Fokus liegt auf:
- einer **steckfertigen ISOBUS-Hardware**
- reproduzierbarer Nutzung im Labor- und Werkstattumfeld
- realistischer Simulation einer ISOBUS-Maschine

---

## Hardware-Übersicht

Der ISOBUS Dummy basiert auf einem **ESP32** als zentraler Recheneinheit und wurde vollständig eigenentwickelt – von der Schaltung bis zum Gehäuse.

### Leiterplatte
- Entwicklung mit [**KiCad**](https://www.kicad.org/)
- ISOBUS-Anbindung über CAN-BUS
- Spannungsversorgung über den ISOBUS
- Schnittstellen für Debugging, Flashing und Erweiterungen

### Gehäuse
- Konstruktion mit **Fusion 360**
- Kompaktes, steckernahes Design
- Mechanischer Schutz der Elektronik
- Integration von Platine und ISOBUS-Stecker in einer Einheit

---

## Leiterplatte 

Das KiCad Projekt befindet sich in ```/PCB``` die entsprechenden Gerber-Dateien befinden sich in ```/PCB/production``` die **.Zip** kann für den upload bei [**JLCPCB**](https://jlcpcb.com/) direkt verwendet werden. <br>
<br>
Die verbauten Komponenten findest du in diesem interaktiven [**BOM**](https://lukashelmke.github.io/ISOBUS_Dummy/ibom.html).<br>
[**CAD der Leiterplatte**](https://lukashelmke.github.io/ISOBUS_Dummy/pcb_model.html)

<p align="center">
  <img src="docs\ISOBUS_Dummy_PCB_Rendering1.PNG" width="3000">
</p>


---

## Gehäuse

Das Fusion360 Projekt zu dem passenden Gehäuse befindet sich in ```/CAD``` <br>
[**CAD vom Gehäuse / Zusammenbau**](https://lukashelmke.github.io/ISOBUS_Dummy/model.html)
<p align="center">
  <img src="docs\ISOBBUS_Dummy_Rendering4.PNG" width="3000">
</p>

