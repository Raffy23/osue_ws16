/*** THIS FILE ONLY EXISTS SO DOXYGEN IS MAKING A NICE MAINPAGE ***/


/*! \mainpage
 * 
 * Es soll ein Programm geschrieben werden, dass die Geldtransfers der Kunden verwaltet. Die Kunden
 * können sich mit dem Programm verbinden und ihre Geldgeschäfte erledigen. Das Verwaltungsprogramm
 * der Bank ist der Server und dieser kommuniziert mittels Shared Memory mit den Clients, die von den
 * Kunden benutzt werden.
 *
 * Die Clients können Geld abheben und einzahlen, es auf ein anderes Konto überweisen und den aktuellen
 * Kontostand abfragen. Aus Gründen der Sicherheit und des Datenschutzes müssen alle Informationen
 * beim Server bleiben und die Clients wissen nur ihre eigene Kontonummer. Außerdem darf pro Konto nur
 * ein Client angemeldet sein und jeder Client soll sich auch abmelden, wenn er sich beendet.
 * 
 * \htmlonly
 * <embed src="banking_td.pdf" width="100%" heigth="800px" href="banking_td.pdf"></embed> 
 * \endhtmlonly
 * 
 * @brief A banking client / server system
 * @author Raphael Ludwig (e1526280)
 * @version 1
 * @date 12 Dez. 2016
 */
