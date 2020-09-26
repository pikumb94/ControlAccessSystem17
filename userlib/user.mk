USERLIB = ./userlib
# List of all the Userlib files
USERSRC =  $(USERLIB)/src/lcd.c \
$(USERLIB)/src/mfrc522.c\
$(USERLIB)/src/motor.c\
$(USERLIB)/src/rfidcodes.c
		   
          
# Required include directories
USERINC =  $(USERLIB) \
           $(USERLIB)/include 
