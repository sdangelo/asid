0 POKE 770,131:POKE 771,164: REM AUTORUN
1 REM SETUP PARAMETERS
10 F=2047 : REM CUTOFF
20 R=0 : REM RESONANCE
30 V=15 : REM VOLUME
40 M=1 : REM MODE
100 REM PRINT FIXED STUFF
110 PRINT CHR$(147)
120 PRINT " F1: CUTOFF (0-2047)  "F : PRINT
130 PRINT " F3: RESONANCE (0-15) "R : PRINT
140 PRINT " F5: VOLUME (0-15)    "V : PRINT
150 PRINT " F7: MODE (0-7)       "M : PRINT
160 PRINT " (MODE BIT 0 = LP, 1 = BP, 2 = HP)"
200 REM UPDATE SID
210 POKE 54293,F AND 7 : REM CUTOFF LOW
220 POKE 54294,INT(F/8) : REM CUTOFF HIGH
230 POKE 54295,(R*16) OR 8 : REM RESONANCE
240 POKE 54296,(M*16) OR V : REM MODE AND VOLUME
250 REM SAW FROM VOICE 1 VARIATION
251 REM POKE 54272+5,0
252 REM POKE 54272+6,240
253 REM POKE 54272+4,33
254 REM POKE 54272+0,0
255 REM POKE 54272+1,16
256 REM POKE 54295,(R*16) OR 9 : REM RESONANCE
260 REM FILTER BYPASS VARIATION
261 REM POKE 54296,R*16
300 REM GET F KEY
310 GET A$ : IF A$="" THEN 310
320 IF A$=CHR$(133) THEN C=0
330 IF A$=CHR$(134) THEN C=1
340 IF A$=CHR$(135) THEN C=2
350 IF A$=CHR$(136) THEN C=3
400 REM GET NUMBER
410 PRINT : PRINT
420 IF C=0 THEN PRINT "NEW CUTOFF"
430 IF C=1 THEN PRINT "NEW RESONANCE"
440 IF C=2 THEN PRINT "NEW VOLUME"
450 IF C=3 THEN PRINT "NEW MODE"
460 INPUT A
470 A=INT(A)
480 IF A<0 THEN A=0
490 IF C=0 AND A>2047 THEN A=2047
500 IF (C=1 OR C=2) AND A>15 THEN A=15
510 IF C=3 AND A>7 THEN A=7
520 IF C=0 THEN F=A
530 IF C=1 THEN R=A
540 IF C=2 THEN V=A
550 IF C=3 THEN M=A
600 REM LOOP
610 GOTO 100
