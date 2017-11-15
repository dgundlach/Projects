IDENTIFICATION DIVISION.                                               
PROGRAM-ID.         HP2RTF.                                            
DATE-WRITTEN.       02/22/2006 -- 03/03/2006.                          
AUTHOR.             DAVID POWELL,  MMfab, Inc.                         
ENVIRONMENT      DIVISION.                                             
CONFIGURATION    SECTION.                                              
INPUT-OUTPUT     SECTION.                                              
FILE-CONTROL.                                                          
    SELECT  RTF-IN-FILE     ASSIGN  "RTFFROM".                         
    SELECT  RTF-OUT-FILE    ASSIGN  "RTFTO".                           
DATA    DIVISION.                                                      
FILE    SECTION.                                                       
                                                                        
$DEFINE  %MAXINLEN=600#                                                 
$DEFINE  %MAXOUTLEN=1024#                                               
                                                                        
FD  RTF-IN-FILE     RECORD  VARYING FROM  0  TO  %MAXINLEN             
                    DEPENDING ON  INPUT-LEN.                           
01  RTF-IN-REC              PIC  X(%MAXINLEN).                         
                                                                        
FD  RTF-OUT-FILE    RECORD  VARYING FROM  0  TO  %MAXOUTLEN            
                    DEPENDING ON    RTF-REC-LEN.                       
01  RTF-OUT-REC             PIC  X(%MAXOUTLEN).                        
                                                                        
WORKING-STORAGE SECTION.                                               
77  INPUT-LEN               PIC S9(09)  COMP.                          
77  RTF-REC-LEN             PIC S9(09)  COMP.                          
77  OUT-PTR                 PIC S9(09)  COMP.                          
77  JUNK                    PIC S9(09)  COMP.                          
77  SUB                     PIC S9(09)  COMP.                          
77  SPACING-CODES           PIC  X(252) VALUE  ALL  "\par".            
77  ERROR-VAR-NAME          PIC  X(10)  VALUE  "RTF_ERRORS".           
77  ERROR-CNT               PIC S9(09)  COMP  VALUE  0.                
77  STDLIST-OR-NOT          PIC  X(01).                                
    88  IS-STDLIST                        VALUE  "I".                    
    88  NOT-STDLIST                       VALUE  "N".                    
77  PRESPACE-OR-POSTSPACE   PIC  X(01)  VALUE  "?".                    
   88  WANT-PRESPACE                     VALUE  "R".                    
   88  WANT-POSTSPACE                    VALUE  "O".                    
   88  WANT-WHAT-SPACING-UNCLEAR         VALUE  "?".                    
                                                                        
01  PCL-CODES-ETC.                                                     
    05  ESC                 PIC  X(01)  VALUE  %33.                    
    05  PCL-BOLD-ON         PIC  X(04)  VALUE  "(s3B".                 
    05  PCL-BOLD-OFF        PIC  X(04)  VALUE  "(s0B".                 
    05  PCL-ITALICS-ON      PIC  X(04)  VALUE  "(s1S".                 
    05  PCL-ITALICS-OFF     PIC  X(04)  VALUE  "(s0S".                 
    05  PCL-UNDER-ON        PIC  X(03)  VALUE  "&dD".                  
    05  PCL-UNDER-OFF       PIC  X(03)  VALUE  "&d@".                  
                                                                        
**   See Intrinsics manual page 4-262 / 4-265 for CCTL codes            
01  CCTL-STUFF.                                                        
    05  F               PIC  X(01)  VALUE  LOW-VALUES.                 
    05  CCTL-BYTE       PIC  X(01).                                    
01  CCTL-CODE   REDEFINES   CCTL-STUFF  PIC S9(04)  COMP.              
    88  CCTL-TRIPLE             VALUE   45.                            
    88  CCTL-DOUBLE             VALUE   48.                            
**   88  CCTL-CONDITIONAL-FF     VALUE   49.                            
    88  CCTL-FF                 VALUE   49,  192.                      
    88  CCTL-WANT-POSTSPACE     VALUE   64.                            
    88  CCTL-WANT-PRESPACE      VALUE   65.                            
**   88  CCTL-NO-AUTO-PAGE-EJECT VALUE   67.                            
    88  CCTL-ZERO               VALUE  128,  43.                       
    88  CCTL-SINGLE             VALUE  129.                            
**   88  CCTL-ADVANCE-63         VALUE  191.                            
    88  CCTL-NORMAL-SPACING     VALUE  129  THRU  191.                 
    88  CCTL-NO-SPACE-NO-RETURN VALUE  208.                            
    88  CCTL-SKIP-IF-EMPTY      VALUE   64, 65, 67.                    
                                                                        
01  GETINFO-FIELDS.                                                    
    05  INFO-STRING         PIC  X(04)  VALUE  SPACES.                 
    05  INFO-LENGTH         PIC S9(04)  COMP   VALUE  4.               
    05  PARM-FILE-CODE      PIC S9(04)  COMP.                          
        88  PARM-FILE-CODE-SPOOL          VALUE  1516.                   
    05  GETINFO-RESULT      PIC S9(04)  COMP.                          
                                                                        
01  GETVAR-FIELDS.                                                     
    05  GETVAR-MAX-LEN          PIC S9(09)  COMP.                      
    05  GETVAR-LEN              PIC S9(09)  COMP.                      
    05  GETVAR-STATUS-X.                                               
        09  GETVAR-ERROR        PIC S9(04)  COMP.                      
        09  GETVAR-SUBSYS       PIC S9(04)  COMP.                      
    05  F       REDEFINES   GETVAR-STATUS-X     PIC  X(04).            
        88  GETVAR-OK         VALUE  LOW-VALUES.                         
$DEFINE  %GETVARINT=                                                    
$CONTROL  LOCOFF                                                        
    CALL  INTRINSIC  "HPCIGETVAR"  USING  !1,  GETVAR-STATUS-X         
                                           \1\, !2                      
    IF  NOT  GETVAR-OK                                                 
        MOVE  0             TO  !2                                     
    END-IF                                                             
$CONTROL  LOCON                                                         
         #                                                              
$DEFINE  %PUTVARINT=                                                    
$CONTROL  LOCOFF                                                        
     CALL  INTRINSIC  "HPCIPUTVAR"  USING  !1,  GETVAR-STATUS-X         
                                           \1\, !2                      
$CONTROL  LOCON                                                         
         #                                                              
**                                                                      
**************************************************                      
**************************************************                      
**                                                                      
 PROCEDURE  DIVISION.                                                   
 0000-MAIN-RTN               SECTION.                                   
     OPEN    INPUT       RTF-IN-FILE.                                   
     OPEN    EXTEND      RTF-OUT-FILE.                                  
                                                                        
     CALL  INTRINSIC  "GETINFO"  USING   INFO-STRING                    
                                         INFO-LENGTH                    
                                         PARM-FILE-CODE                 
                                GIVING   GETINFO-RESULT.                
     IF  GETINFO-RESULT      <>  0                                      
         DISPLAY  "'GETINFO' INTRINSIC FAILURE; RESULT = "              
                  GETINFO-RESULT                                        
                  "; INFO-STRING = ",  INFO-STRING                      
                  "; INFO-LENGTH = ",  INFO-LENGTH                      
                  "; PARM = ",  PARM-FILE-CODE                          
         CALL  INTRINSIC  "QUIT"  USING  \1\.                           
                                                                        
     PERFORM  1000-DETAIL-LOOP.                                         
     IF  ERROR-CNT       <>  0                                          
         %GETVARINT(ERROR-VAR-NAME#,JUNK#)                              
         IF  GETVAR-OK                                                  
             ADD  ERROR-CNT      TO  JUNK                               
             %PUTVARINT(ERROR-VAR-NAME#,JUNK#)                          
         END-IF                                                         
         IF  NOT  GETVAR-OK                                             
             DISPLAY  "HAD JCW ERRORS; ERROR-COUNTER VARIABLE "         
                      "MIGHT NOT BE CORRECT".                           
                                                                        
     CLOSE   RTF-IN-FILE                                                
             RTF-OUT-FILE.                                              
     STOP RUN.                                                          
 0000-MAIN-X.  EXIT.                                                    
**                                                                      
*************************************                                   
**                                                                      
 1000-DETAIL-LOOP            SECTION.                                   
**   Skip 1 or 2 junk records at start of a CCTL file; then             
**   read 1st rec, and convert form-feed (& unknown cctl?) to           
**   no-space.                                                          
**                                                                      
**   Always eat the 1st rec of a spool file (forms-msg or empty)        
**                                                                      
     IF  PARM-FILE-CODE-SPOOL                                           
         READ  RTF-IN-FILE,  AT END                                     
             DISPLAY  "ERROR READING SPOOL-FILE 1ST REC"                
             ADD  1          TO  ERROR-CNT                              
             GO TO  1000-WRITE-FINAL.                                   
                                                                        
 1000-READ-FIRST-REAL-REC.                                              
     READ  RTF-IN-FILE,  AT  END                                        
         DISPLAY "ERROR READING 1ST REC"                                
         ADD  1              TO  ERROR-CNT                              
         GO TO  1000-WRITE-FINAL.                                       
                                                                        
     IF  INPUT-LEN           =   0                                      
         GO TO   1000-READ-FIRST-REAL-REC.                              
                                                                        
     MOVE  RTF-IN-REC(1:1)       TO  CCTL-BYTE.                         
     IF  CCTL-WANT-POSTSPACE                                            
         SET     WANT-POSTSPACE      TO  TRUE                           
     ELSE,   IF  CCTL-WANT-PRESPACE                                     
         SET     WANT-PRESPACE       TO  TRUE.                          
     IF  ( INPUT-LEN             =   1 )                                
     OR  ( RTF-IN-REC(2: INPUT-LEN - 1)  =   SPACES )                   
         IF  CCTL-SKIP-IF-EMPTY    OR   CCTL-FF                         
             GO TO  1000-READ-FIRST-REAL-REC.                           
                                                                        
     IF  NOT  PARM-FILE-CODE-SPOOL                                      
*---*    DISPLAY  "non-spool file"                                      
         SET  NOT-STDLIST        TO  TRUE                               
     ELSE,   IF  RTF-IN-REC(2:5)     =   ":JOB "                        
*---*    DISPLAY  "stdlist"                                             
         SET  IS-STDLIST         TO  TRUE                               
     ELSE                                                               
*---*    DISPLAY  "spool file (not stdlist)"                            
         SET  NOT-STDLIST        TO  TRUE.                              
                                                                        
     IF  WANT-WHAT-SPACING-UNCLEAR                                      
         IF  ( PARM-FILE-CODE-SPOOL  AND     NOT-STDLIST )              
             SET     WANT-PRESPACE       TO  TRUE                       
         ELSE                                                           
             SET     WANT-POSTSPACE      TO  TRUE.                      
                                                                        
     IF  ( NOT CCTL-NORMAL-SPACING )                                    
     AND ( WANT-PRESPACE )                                              
         SET  CCTL-ZERO          TO  TRUE.                              
                                                                        
 1000-PROCESS-REC.                                                      
**   zap trailing blanks in input                                       
     IF  NOT ( CCTL-NO-SPACE-NO-RETURN   AND   WANT-POSTSPACE )         
         PERFORM     UNTIL   INPUT-LEN       <=  1                      
                     OR      RTF-IN-REC(INPUT-LEN:1) <>  " "            
             SUBTRACT  1     FROM    INPUT-LEN                          
         END-PERFORM.                                                   
     IF  ( CCTL-SKIP-IF-EMPTY )                                         
         IF  ( INPUT-LEN   <=  1 )                                      
         OR  ( RTF-IN-REC(2: INPUT-LEN - 1)  =   SPACES )               
             GO TO  1000-READ-NEXT-REC.                                 
                                                                        
     MOVE  1                 TO  OUT-PTR.                               
     IF  WANT-PRESPACE                                                  
         PERFORM  1100-SPACING                                          
         PERFORM  1200-CONTENTS                                         
     ELSE                                                               
         PERFORM  1200-CONTENTS                                         
         PERFORM  1100-SPACING.                                         
                                                                        
                                                                        
**   Write output                                                       
     COMPUTE  RTF-REC-LEN    =   OUT-PTR - 1.                           
     WRITE  RTF-OUT-REC.                                                
                                                                        
**   Read next input                                                    
 1000-READ-NEXT-REC.                                                    
     READ    RTF-IN-FILE                                                
         AT END,     GO TO   1000-WRITE-FINAL.                          
                                                                        
     IF  INPUT-LEN       =   0                                          
         GO TO  1000-READ-NEXT-REC                                      
*???*    SET  CCTL-SINGLE        TO  TRUE                               
     ELSE                                                               
         MOVE  RTF-IN-REC(1:1)   TO  CCTL-BYTE                          
         IF  CCTL-WANT-POSTSPACE                                        
             IF  WANT-PRESPACE                                          
                 SET  CCTL-SINGLE        TO  TRUE                       
             END-IF                                                     
             SET     WANT-POSTSPACE      TO  TRUE                       
         ELSE,   IF  CCTL-WANT-PRESPACE                                 
             SET     WANT-PRESPACE       TO  TRUE.                      
                                                                        
     GO TO  1000-PROCESS-REC.                                           
                                                                        
 1000-WRITE-FINAL.                                                      
     MOVE  "}"       TO  RTF-OUT-REC.                                   
     MOVE  1         TO  RTF-REC-LEN.                                   
     WRITE   RTF-OUT-REC.                                               
 1000-DETAIL-X.  EXIT.                                                  
**                                                                      
*****************************************                               
**                                                                      
 1100-SPACING                SECTION.                                   
**   Set RTF equiv of the CCTL code; set OUT-PTR to next byte.          
     IF  CCTL-NORMAL-SPACING                                            
         COMPUTE  JUNK   =   ( CCTL-CODE - 128 )  *  4                  
         MOVE  SPACING-CODES(1:JUNK)                                    
                                 TO  RTF-OUT-REC(OUT-PTR:JUNK)          
         ADD  JUNK       TO  OUT-PTR                                    
         MOVE  " "       TO  RTF-OUT-REC(OUT-PTR:1)                     
         ADD     1       TO  OUT-PTR                                    
         GO TO  1100-DONE-SPACING-X.                                    
     IF  CCTL-FF                                                        
         MOVE  "\page "  TO  RTF-OUT-REC(OUT-PTR:6)                     
         ADD   6         TO  OUT-PTR                                    
         GO TO  1100-DONE-SPACING-X.                                    
     IF  CCTL-NO-SPACE-NO-RETURN                                        
         GO TO  1100-DONE-SPACING-X.                                    
                                                                        
**   Ideally, would have sth below to make it go to column 1            
**   and overprint.                                                     
     IF  CCTL-ZERO                                                      
         GO TO  1100-DONE-SPACING-X.                                    
                                                                        
     IF  CCTL-DOUBLE                                                    
         STRING  "\par\par "                                            
             DELIMITED  BY  SIZE     INTO    RTF-OUT-REC                
             WITH    POINTER     OUT-PTR                                
         GO TO  1100-DONE-SPACING-X.                                    
     IF  CCTL-TRIPLE                                                    
         STRING  "\par\par\par "                                        
             DELIMITED  BY  SIZE     INTO    RTF-OUT-REC                
             WITH    POINTER     OUT-PTR                                
         GO TO  1100-DONE-SPACING-X.                                    
                                                                        
                                                                        
**                                                                      
**   If there are any codes that should be recognized only in           
**   non-spool files, activate the following, and put the codes         
**   in question AFTER it.                                              
**                                                                      
*===*IF  PARM-FILE-CODE-SPOOL                                           
*===*    MOVE  "\par "       TO  RTF-OUT-REC(OUT-PTR:5)                 
*===*    ADD   5             TO  OUT-PTR                                
*===*    GO TO  1100-DONE-SPACING-X.                                    
                                                                        
                                                                        
     STRING  "\par "                                                    
         DELIMITED  BY  SIZE     INTO    RTF-OUT-REC                    
         WITH    POINTER     OUT-PTR.                                   
 1100-DONE-SPACING-X.  EXIT.                                            
**                                                                      
********************************************                            
**                                                                      
 1200-CONTENTS                       SECTION.                           
**   Build the rest of the output rec, copying input & converting       
**   some PCL esc-codes to RTF as we go.                                
                                                                        
*__*                                                                    
*__* Following is the 'SLOW' version that converts some special         
*__* codes like boldface, underlining, italics, \ { and }.              
*__*                                                                    
     MOVE  2             TO  SUB.                                       
 1200-NEXT-BYTE.                                                        
     IF  SUB             <=  INPUT-LEN                                  
         IF  RTF-IN-REC(SUB:1)       =   ESC                            
             IF  RTF-IN-REC(SUB + 1:4)   =   PCL-BOLD-ON                
                 MOVE  "\b "         TO  RTF-OUT-REC(OUT-PTR:3)         
                 ADD   3             TO  OUT-PTR                        
                 ADD   5             TO  SUB                            
                 GO  TO  1200-NEXT-BYTE                                 
             END-IF                                                     
             IF  RTF-IN-REC(SUB + 1:4)   =   PCL-BOLD-OFF               
                 MOVE  "\b0 "        TO  RTF-OUT-REC(OUT-PTR:4)         
                 ADD   4             TO  OUT-PTR                        
                 ADD   5             TO  SUB                            
                 GO  TO  1200-NEXT-BYTE                                 
             END-IF                                                     
                                                                        
             IF  RTF-IN-REC(SUB + 1:4)   =   PCL-ITALICS-ON             
                 MOVE  "\i "         TO  RTF-OUT-REC(OUT-PTR:3)         
                 ADD   3             TO  OUT-PTR                        
                 ADD   5             TO  SUB                            
                 GO  TO  1200-NEXT-BYTE                                 
             END-IF                                                     
             IF  RTF-IN-REC(SUB + 1:4)   =   PCL-ITALICS-OFF            
                 MOVE  "\i0 "        TO  RTF-OUT-REC(OUT-PTR:4)         
                 ADD   4             TO  OUT-PTR                        
                 ADD   5             TO  SUB                            
                 GO  TO  1200-NEXT-BYTE                                 
             END-IF                                                     
                                                                        
             IF  RTF-IN-REC(SUB + 1:3)   =   PCL-UNDER-ON               
                 MOVE  "\ul "        TO  RTF-OUT-REC(OUT-PTR:4)         
                 ADD   4             TO  OUT-PTR                        
                 ADD   4             TO  SUB                            
                 GO  TO  1200-NEXT-BYTE                                 
             END-IF                                                     
             IF  RTF-IN-REC(SUB + 1:3)   =   PCL-UNDER-OFF              
                 MOVE  "\ul0 "       TO  RTF-OUT-REC(OUT-PTR:5)         
                 ADD   5             TO  OUT-PTR                        
                 ADD   4             TO  SUB                            
                 GO  TO  1200-NEXT-BYTE                                 
             END-IF                                                     
         END-IF                                                         
                                                                        
         IF  RTF-IN-REC(SUB:1)       =   "\"                            
             MOVE  "\\"              TO  RTF-OUT-REC(OUT-PTR:2)         
             ADD  2                  TO  OUT-PTR                        
             ADD  1                  TO  SUB                            
             GO TO  1200-NEXT-BYTE                                      
         END-IF                                                         
         IF  RTF-IN-REC(SUB:1)       =   "{"                            
             MOVE  "\{"              TO  RTF-OUT-REC(OUT-PTR:2)         
             ADD  2                  TO  OUT-PTR                        
             ADD  1                  TO  SUB                            
             GO TO  1200-NEXT-BYTE                                      
         END-IF                                                         
         IF  RTF-IN-REC(SUB:1)       =   "}"                            
             MOVE  "\}"              TO  RTF-OUT-REC(OUT-PTR:2)         
             ADD  2                  TO  OUT-PTR                        
             ADD  1                  TO  SUB                            
             GO TO  1200-NEXT-BYTE                                      
         END-IF                                                         
                                                                        
         MOVE  RTF-IN-REC(SUB:1)     TO  RTF-OUT-REC(OUT-PTR:1)         
         ADD   1                     TO  SUB,    OUT-PTR                
         GO  TO  1200-NEXT-BYTE                                         
     END-IF.                                                            
*__*                                                                    
*__*     Preceeding is the "official" version that converts some        
*__*     special codes; following is the faster code that does          
*__*     not.  One of them must be active and the other must be         
*__*     commented-out.                                                 
*__*                                                                    
*    COMPUTE     JUNK    =   INPUT-LEN  -  1.                           
*    IF  JUNK    >   0                                                  
*        MOVE  RTF-IN-REC(2:JUNK)    TO  RTF-OUT-REC(OUT-PTR:JUNK)      
*        ADD   JUNK                  TO  OUT-PTR.                       
*__*                                                                    
*__* Preceeding is the 'fast' version that converts nothing but         
*__* carriage-control codes.  It may fail big-time on files with        
*__* "\", "{" or "}" in them, and also won't handle boldface,           
*__* italics, or underlining, etc.  Use at your own risk.               
*__*                                                                    
 1200-CONTENTS-X.  EXIT.
