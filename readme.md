# Age of Mythology: The Titan recording file parser  
C++ program that analyze the content of .rcx files and create a summary of the actions taken by each player in the match. Actions include: move commands, units trained, researches completed, building contructed, units garrisoned, resources sent/received from ally, resources bought/sold, ecc...  
The parser also extracts the game settings and the scripts used to create the map.  
  
## Requirements  
Python (for zlib) to decompress the .rcx file  

## Not supported  
The recording file only contains the actions taken by the players during the match. This means that it's not possible to retrieve any information that require some degree of simulation, such as: resources gathered and kills.  
Moreover, althought feasible, at the current time the parser is not able to show any type of unit-specific statistics (i.e. villagers trained, town center contructed ecc..).

