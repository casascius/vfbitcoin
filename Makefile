#------------------------------------------------------------ 
#
#       NMAKE file to build vfbitcoin
#
#-------------------------------------------------------------

#
# Paths
#

# SDS Tools Related Paths

SDSIncludes = C:\VerixVAps\VRXSDK\include
VRXSDK = C:\VerixVAps\VRXSDK
ACT2000 = C:\VerixVAps\ACT2000

#  Compiler/Linker/Outhdr Output Paths
#D:\a\vf\verix\757

ObjDir = $(SOURCE)

FinalAOutFileName = vfbtc.out

# Also do not forget to update App757.fst to sign the correct .OUT file.
FinalAP7SFileName = vfbtc.p7s 
OSfile = D:\a\vf\verix\757\Gid0


#
# Options for Tools
#

ACTLibraries= C:\VerixVAps\ACT2000\OutPut\RV\Files\Static\Debug
UCLIncludes = $(VUCL)\Include
TCPIPIncludes = $(VTCPIP)\include
ACTIncludes = C:\VerixVAps\ACT2000\Include
VUCLLibraries =C:\VerixVAps\UCL\OutPut\RV\Files\Static\Debug
VTCPLibraries = C:\VerixVAps\TCPIP\OutPut\RV\Files\Static\Debug

# Compiler Options
Includes = -I$(SDSIncludes) -I$(UCLIncludes) -I$(TCPIPIncludes) -I$(ACTIncludes)
# for release version change the COptions to 
COptions =  -DLOGSYS_FLAG

#
# Dependencies
#

AppObjects = btcui.o lcdgraphics.o bitmaps.o VerixPlatform.o bitstream.o mask.o mmask.o mqrspec.o qrencode.o qrinput.o qrspec.o rscode.o split.o
AppHeaders = lcdgraphics.h platform.h


Libs =	 \
$(ACTLibraries)\act2000.a \
$(VUCLLibraries)\ucl.a	\
$(VTCPLibraries)\vtcpip.a	\
$(VTCPLibraries)\vSSL.a	\
$(VTCPLibraries)\vCrypto.a	
#$(VRXSDK)\lib\verix.lib 


#
#  sample Target Definition
#
# The -d option of vrxhdr is to enable debugging.  Remove it for release configuration. 

pseudoOut : $(FinalAOutFileName)
#	Important Stack and Heap sizes, TCP/IP and SSL need them in order to work fine and with Threads.
#   REASON for a small stack size - this sets the stack for the main (UI) thread.
#   The TCP/IP library will be run entirely by the 2nd thread, whose stack is (I believe) taken from the heap when created.
#   TCP/IP with SSL on Vx570 needs 120KB heap and 27KB stack (which will come off the heap at run_thread).
#   So UI thread can have 24 KB stack.  COMM thread can have 48KB stack and 120KB heap, total 168KB heap, plus a little margin.
	C:\VerixVAps\VRXSDK\bin\vrxhdr -s 32000 -h 48000 $(FinalAOutFileName)
	

#Use the following lines to sign the out file. 

	"%VSFSTOOL%\filesignature.exe" "vfbtc.fst" -nogui
	copy $(FinalAOutFileName).p7s $(FinalAP7SFileName)
	del $(FinalAOutFileName).p7s



$(FinalAOutFileName) : $(AppObjects)
	$(VRXSDK)\bin\vrxcc -g $(AppObjects) $(Libs) -o $(FinalAOutFileName)


 
######  Compile #######
# The -g option of vrxcc is to enable debugging symbols and output.  Remove it for
# normal usage.

config.sys : config.txt
    $(VRXSDK)\bin\vlr -c config.txt config.sys

VerixPlatform.o: VerixPlatform.c $(AppHeaders)
    $(VRXSDK)\bin\vrxcc -c $(Includes)  $(COptions) VerixPlatform.c
	
btcui.o : btcui.c $(AppHeaders)
    $(VRXSDK)\bin\vrxcc -c $(Includes)  $(COptions) btcui.c

lcdgraphics.o : lcdgraphics.c $(AppHeaders)
    $(VRXSDK)\bin\vrxcc -c $(Includes)  $(COptions) lcdgraphics.c

globals.o : globals.c $(AppHeaders) 
	$(VRXSDK)\bin\vrxcc -c $(Includes)  $(COptions) globals.c

sf600.o : sf600.c $(AppHeaders)
	$(VRXSDK)\bin\vrxcc -c $(Includes)  $(COptions) sf600.c

bitmaps.o : bitmaps.c $(AppHeaders) 
	$(VRXSDK)\bin\vrxcc -c $(Includes)  $(COptions) bitmaps.c

bitstream.o : bitstream.c $(AppHeaders) 
	$(VRXSDK)\bin\vrxcc -c $(Includes)  $(COptions) bitstream.c
	
mask.o : mask.c $(AppHeaders) 
	$(VRXSDK)\bin\vrxcc -c $(Includes)  $(COptions) mask.c
	
mmask.o : mmask.c $(AppHeaders) 
	$(VRXSDK)\bin\vrxcc -c $(Includes)  $(COptions) mmask.c

mqrspec.o : mqrspec.c $(AppHeaders) 
	$(VRXSDK)\bin\vrxcc -c $(Includes)  $(COptions) mqrspec.c
	
#qrenc.o : qrenc.c $(AppHeaders) 
#	$(VRXSDK)\bin\vrxcc -c $(Includes)  $(COptions) qrenc.c

qrencode.o : qrencode.c $(AppHeaders) 
	$(VRXSDK)\bin\vrxcc -c $(Includes)  $(COptions) qrencode.c

qrinput.o : qrinput.c $(AppHeaders) 
	$(VRXSDK)\bin\vrxcc -c $(Includes)  $(COptions) qrinput.c
	
qrspec.o : qrspec.c $(AppHeaders) 
	$(VRXSDK)\bin\vrxcc -c $(Includes)  $(COptions) qrspec.c

rscode.o : rscode.c $(AppHeaders) 
	$(VRXSDK)\bin\vrxcc -c $(Includes)  $(COptions) rscode.c

split.o : split.c $(AppHeaders) 
	$(VRXSDK)\bin\vrxcc -c $(Includes)  $(COptions) split.c
	
	