%   � 770,131:� 771,164: � AUTORUN =  � SETUP PARAMETERS T
  F�2047 : � CUTOFF k  R�0 : � RESONANCE �  V�15 : � VOLUME �(  M�1 : � MODE �d  � PRINT FIXED STUFF �n  � �(147) �x  � " F1: CUTOFF (0-2047)  "F : � 	�  � " F3: RESONANCE (0-15) "R : � (	�  � " F5: VOLUME (0-15)    "V : � M	�  � " F7: MODE (0-7)       "M : � y	�  � " (MODE BIT 0 = LP, 1 = BP, 2 = HP)" �	�  � UPDATE SID �	�  � 54293,F � 7 : � CUTOFF LOW �	�  � 54294,�(F�8) : � CUTOFF HIGH �	�  � 54295,(R�16) � 8 : � RESONANCE #
�  � 54296,(M�16) � V : � MODE AND VOLUME E
�  � SAW FROM VOICE 1 VARIATION V
�  � 54272�5,0 i
�  � 54272�6,240 {
�  � 54272�4,33 �
�  � 54272�0,0 �
�  � 54272�1,16 �
  � 54295,(R�16) � 9 : � RESONANCE �
 � FILTER BYPASS VARIATION �
 � POKE 54296,R*16 , � GET F KEY %6 � A$ : � A$�"" � 310 <@ � A$��(133) � C�0 SJ � A$��(134) � C�1 jT � A$��(135) � C�2 �^ � A$��(136) � C�3 �� � GET NUMBER �� � : � �� � C�0 � � "NEW CUTOFF" �� � C�1 � � "NEW RESONANCE" �� � C�2 � � "NEW VOLUME" � � C�3 � � "NEW MODE" � � A $� A��(A) 5� � A�0 � A�0 R� � C�0 � A�2047 � A�2047 s� � (C�1 � C�2) � A�15 � A�15 �� � C�3 � A�7 � A�7 � � C�0 � F�A � � C�1 � R�A � � C�2 � V�A �& � C�3 � M�A �X � LOOP �b � 100   