Arduino
================

## Embedded Computing System project - Arduino implementation. 

Language: C

This program executes two main things:
- Heart beat data aquisition:
    - allowGetHeartBeat, called by interrupts at every heart beat allows getHeartBeat to be called;
    - getHeartBeat, gets single heart beat time and every # of beats get heart rate;
    - heartBeatData, uses heart rates to analyse sleep;
- Serial messages aquisition:
    - timeWithoutMovement, gets time without movement data and uses it to change heart rate lowering percentage to go to sleep state
    - switchLight, switchs light state
    - reallyRoughMovement, restart system if lamp goes off by sleep state detection

Dependencies:

1.  Arduino: http://www.arduino.cc/
		
Running:
- open arduino.exe
- open ECS_DetectSleep.ino inside arduino.exe
- compile and upload code via USB to arduino

Collaborators:
- Tiago Pimentel Martins da Silva - t.pimentelms@gmail.com
- Joao Felipe Nicolaci Pimentel	- joaofelipenp@gmail.comy
- Matheus Fernandes de Camargo	- matheusfc09@gmail.com
