# CR_Tally

Objectiu: 
Crear tallys (pilots vermells ON AIR) inhalambrics multiproposit.

Hardware:
Es basarà en ESP32 DEV Kit utilitzant el protocol de comunicació de la casa Espressif ESPNow per connectar els diferents dispositius.
Aquest protocol permet una ràpida connexió utilitzant la banda lliure de 2,4GHz amb un abast superior al WIFI convencional.

Sobre una base estandard es crearàn tres dispositius aparentmet identics però amb funcionalitats diferents: el controlador, la posició de conductor, la posició de productor, la posició de convidat.

HARDWARE BASE:
Carcassa de plàstic: Capsa de plàstic impressa en impressora 3D.
Disseny pendent

Placa d'alimentació amb dos bateries del tipus 18650 que suministren 3,3V i 5V i fan funcions de càrregà i protecció d'aquestes. Es careguen via conector USB.
https://www.aliexpress.com/snapshot/0.html?spm=a2g0o.9042647.0.0.6d3763c0rcxVbg&orderId=8145150705073892&productId=1005001854607900

Atenció: Les plaques rebudes no subministren 3V, donen 1,8V a la sortida marcada com a 3V. La sortida marcada a 5V si que treu 5V. Les bateries es carreguen i descareguen en pararl.lel. No fa balanç de càrrega, per tant és important tenir-les ben aparellades!
Cal investigar el commutador hold i normal per veure com afecta a la alimentació. El polsador engega i para el subminsitre de tensió. Es poden utilitzar mentre es carreguen.


Interruptor d'encessa/apagat
https://es.aliexpress.com/item/1005001513148147.html?gatewayAdapt=glo2esp&spm=a2g0o.9042311.0.0.242c63c0Rw7HHY

Placa ESp32:
https://es.aliexpress.com/item/32864722159.html?gatewayAdapt=glo2esp&spm=a2g0o.9042311.0.0.242c63c0Rw7HHY

Display OLED blanc:
https://es.aliexpress.com/item/32672229793.html?gatewayAdapt=glo2esp&spm=a2g0o.9042311.0.0.242c63c0Rw7HHY

Polsadors vermell i verd:
https://es.aliexpress.com/item/1005003120415869.html?gatewayAdapt=glo2esp&spm=a2g0o.9042311.0.0.242c63c0Rw7HHY

Polsadors

    3
    2
A   1   B

Vist des dels connectors:
1- Contact  
2- NO
3- NC
A- + Led
B- - Led

Pilot Led local 8x1:
https://es.aliexpress.com/item/4000195919675.html?gatewayAdapt=glo2esp&spm=a2g0o.9042311.0.0.242c63c0Rw7HHY

Matriu led 8x8 indicadora:
https://es.aliexpress.com/item/4001296811800.html?gatewayAdapt=glo2esp&spm=a2g0o.9042311.0.0.242c63c0Rw7HHY

Level shifter (per adaptar 3,3v a 5V i inversa):
https://es.aliexpress.com/item/1005001839292815.html?gatewayAdapt=glo2esp&spm=a2g0o.9042311.0.0.20cd63c0iFG4tf


MASTER:
El controlador master serà l'encarregat d'intercactuar mitjançant GPIO d'entrada i sortida amb senyals externes provinents de la taula de só local i dels estudis centrals, i al mateix temps enviar GPO per activar funcions de la taula local o senyalitzacions als estudis centrals.  
Fisicament serà identic als altres però inclourà dos connectors de comunicació per als GPIO i tindrà accés mitjançant WIFI a internet per poder-lo configurar i adquirir parametres com el servidor de temps NTP.

Utilitzant el level shifter podem generar un bit per determinar si tenim connectat el connector GPIO de la taula QL o del VIA. caldrà veure si tenim prou ports disponibles.

MASTER: ID: 0
Igual que el Productor. 

CONDUCTOR: ID: 1
Botó vermell dona ordres a productor local, botó verd dona ordres a estudis centrals.

PRODUCTOR: ID: 2
Botó vermell dona ordres a conductor, botó verd dona ordres a estudis centrals.

Que ha de fer:
Quan es rebi GPIO ON AIR dels estudis encendre els pilots en taronja.
Quan rebi GPIO ON de la taula i també dels estudis encendre els pilots en vermell.
Quan rebi només GPIO de la taula i no dels estudis encendre en groc.
Quan no rebi GPIO de la taula ni del estudi encendre en verd.

Estem limnitats als GPIO IN de la QL1 (5 max).
Bit 1 - Lliure
Bit 2 - COND Ordres Productor (vermell)
Bit 3 - COND Ordres Estudi (verd)
Bit 4 - PROD Ordres Conductors (vermell)
Bit 5 - PROD Ordres Estudi (verd)

Utilitzarem els GPIO OUT de la QL1 per confirmar les operacions.
D'aquesta manera quan s'hagi carregat l'escena coresponent enviarem el bit de confirmació als polsadors de tally. Ens queda un bit lliure d'entrada a la QL

GPIO OUT QL 1
Bit 1 - Micro CONDUCTOR ON (confirma tally)
Bit 2 - Confirmació COND Ordres Productor (led vermell polsador COND ON)
Bit 3 - Confirmació COND Ordres Estudi (led verd polsador COND ON)
Bit 4 - Confirmació PROD Ordres Conductors (led vermell polsador PROD ON)
Bit 5 - Confirmació PROD Ordres Estudi (led verd polsador PROD ON)
Bit 6 - Presencia de 5V

Que han de fer les escenes de la QL:

Quan rebem un Bit 1 - RES - Lliure

Quan reben un Bit 2 - CONDUCTOR Ordres a Productor (vermell):
Treure Conductor de PGM i PA, connectar el micro del conductor al MIX del productor(opcionalment atenuar PGM dels auriculars del productor), enviar un bit per la sortida 2.

Quan reben un Bit 3 - CONDUCTOR Ordres a Estudi (verd):
Treure Conductor de PGM i PA, connectar el micro del conductor al MIX de les ordres del productor cap a Estudi, (opcionalment atenuar PGM dels auriculars del conductor), afegir les ordres del estudi al MIX dels auriculars del conductor, enviar un bit per la sortida 3.

Quan reben un Bit 4 - PRODUCTOR Ordres a Conductor (vermell):
Desmutejar micro productor del BUS que va als auriculars del conductor i del productor, (opcionalment atenuar PGM dels auriculars del conductor i del productor - pq aquest s'escolti a si mateix), enviar un bit per la sortida 4.

Quan reben un Bit 5 - PRODUCTOR Ordres Estudi (verd):
Desmutejar micro productor del BUS que va a les ordres del estudi i als auriculars del productor, (opcionalment atenuar PGM dels auriculars del productor - pq aquest s'escolti a si mateix), enviar un bit per la sortida 5.

Operativa del TALLY:

Si no rebem res pel bit 6A (tensio) del VIA (A), missatge indicant DESCONECTAT DEL GPIO A (VIA). 
Si no rebem res pel bit 6B (tensio) de la QL (B), missatge indicant DESCONECTAT DEL GPIO B (QL). 

Si tenim el bit 6A del VIA (A) i no del QL (B) quan rebem el bit 1A (VIA) encendrem el tally en VERMELL. D'aquesta manera podem utilitzar el tally només amb el VIA.

Si tenim el bit 6B de la QL (B) i no del VIA (A) quan rebem el bit 1B (QL) encendrem el tally VERD/BLAU  (LOCAL REC). D'aquesta manera indiquem que estem gravant o enviant per un altre camí. 

Si tenim el bit 6A de (VIA) i el 6B de (QL) - els dos equips conectats:
Si tenim el bit 1A del VIA i el bit 1B de la QL encendrem el tally en VERMELL. (ON AIR)
Si només tenim el bit 1A (VIA) i no el bit 1B (QL) encendrem el tally TARONJA. (PRECAUCIÓ ESTUDI OBERT)
Si només tenim el bit 1B (QL) i no tenim el bit 1A (VIA) encendrem el tally GROC. (ESPERANT ESTUDI).
Si no tenim cap bit 1A ni 1B el tally queda apagat.

GPIO del VIA:

Disposem de 5 GPI i 5 GPO del VIA.

GPIO OUT VIA

Bit 1 - Canal obert
Bit 2 - Lliure
Bit 3 - LLiure
Bit 4 - Lliure
Bit 5 - LLiure
Bit 6 - Presencia 5V 


GPIO IN VIA
Bit 1 - Lliure
Bit 2 - Lliure
Bit 3 - LLiure
Bit 4 - Lliure
Bit 5 - LLiure

Podriem instal.lar un heartbeat per comprovar que la linea és operativa.

Que podria fer: 
El master es connecta a internet i ofereix el temps real al display.
Cada posició pot tenir un numero indicant la posició de la taula. El numero canvi de color.

Programació: 

Les primera versió servirà per provar la cobertura del ESPNow. Pulsant un bit del ESP32 ha d'encendre un led en el recepctor.


Modul Master:

6 GPI IN (QL)
5 GPIO OUT (QL)
6 GPI IN (VIA)
5 GPIO OUT (VIA)
Total: 22 Pins + 8 = 30 Pins Ocupats

Modul Tally:

Inputs:
Polsador Vermell
Polsador verd
Analog IN (Nivell Bateria) Cal fer un petit divisor de tensió

Outputs: 
SDA - Display
SCL - Display
Digital led (Display garn i petit)
Led Vermell
Led Verd

Total: 8 PINS
2 Digital IN
1 Analog IN
2 GPIO (leds)
3 GPIO Matriu led i display

Revisió de funcionament amb la QL:

Farem servir els bits 2,3,4 i 5 d'entrada a la QL per fer: 

Bit 1 - Mute COND
Bit 2 - COND Ordres Productor (vermell)
Bit 3 - COND Ordres Estudi (verd)
Bit 4 - PROD Ordres Conductors (vermell)
Bit 5 - PROD Ordres Estudi (verd)

Ordres a de Conductor a Productor:
Al apretar boto vermell del conductor:
Arriba un 1 al bit 1 (mute COND)
Arriba un 1 al bit 2 (Ordres Prod)

Quan arribi 1 a Bit 1 sempre fa un mute del COND
Quan arribi 1 a Bit 2 es fa unmute de la copia del conductor que va al mix del productor.

Ordres a de Conductor a Estudi:
Al apretar boto verd del conductor:
Arriba un 1 al bit 1 (mute COND)
Arriba un 1 al bit 3 (Ordres Prod)

Quan arribi 1 a Bit 1 sempre fa un mute del COND
Quan arribi 1 a Bit 2 es fa unmute de la copia del conductor que va al mix del ordres estudi. Es fa unmute de les ordres del estudi al conductor.

Ordres a de Productor a Estudi:
Al apretar boto verd del productor:
Arriba un 1 al bit 5 (Ordres EST) 

Fem unmute ordres Productor a Estudi.
Posem retorn atenuat (mute retorn normal)

