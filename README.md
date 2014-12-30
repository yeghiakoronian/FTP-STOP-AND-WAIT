FTP-STOP-AND-WAIT
=================

FTP stop and wait protocol using UDP 

File Transfer Protocol Using "Stop-and-Wait" and one timer
The Stop-and-Wait protocol operates as follows: Each packet of your private protocol   is sent as a UDP packet. If no loss occurs ,
the peer host returns an acknowledgement packet, again using UDP. Once this acknowledgement has been received, the next data packet 
can be sent. If a packet is lost (on the forward path or on the reverse path), then the sending host will time out, and send it again

Notes
-----
After building Client and Server make sure to create folder called files in the root debug  folder ( right click on solution clientprocessass2  then click
open folder in filer explorer , then click debug then once in debug folder create files folder , do the same  for server)
C:\Users\xxxxxx\Desktop\FTP-Stop-and-Wait\ClientProcessAss2\Debug\files