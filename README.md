# Operating Systems UE WS16

## Beispiel 1A - Palindrom
Liest einen String über stdin ein und gibt auf stdout aus ob der string ein Palindorm ist oder nicht

## Beispiel 1B - Mastermind
* 8 Farben, Client muss secret vom Server erraten
* TCP/IP Server und Client
* Protokoll:
  * Client: *(Guess)*
  ```
  | 15 | 14   13   12 | 11   10   9 |  8   7   6  |  5   4   3  |  2   1   0  |
  +----+--------------+-------------+-------------+-------------+-------------+
  | p  |   color R    |    color    |    color    |    color    |   color L   |
  +----+--------------+-------------+-------------+-------------+-------------+
  
   p = paritätsbit
  ```
  * Server: *(Response)*
  ```
  |  7   6 |   5   4   3  |  2   1   0 |
  +--------+--------------+------------+
  | status | number white | number red |
  +--------+--------------+------------+
  
  status (7) = Spiel ist zuende (35. Runde)
  status (6) = Paritätsbit ist falsch
  ```
## Beispiel 2 - Dsort
Programm welches sich wie folgendes Bash Skript verhält:
```bash
#!/bin/bash
( $1; $2 ) | sort | uniq -d
```

## Beispiel 3 - Banking
* Server / Client per Shared Memory
* Server liest aus CSV Daten aus

## Bonus Beispiel (Linux Kernel Module)
* Secure Vault (verschlüsselt Daten mittels XOR)
* Userspace utility welches mit Kernel Module kommuniziert
  * ioctl per `/dev/sv_ctl`
  * data devices `/dev/sv_data[0-3]` lesen / schreiben der Daten
