#------------------------------------------------------------ 
#
#       Makefile to build vfbitcoin
#       Note: VFSDK comes with a proprietary Windows make.  
#       Don't use it.  Use GNU make instead.
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

binary = vfbtc.out
signature = vfbtc.p7s 
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

objects = btcui.o lcdgraphics.o bitmaps.o VerixPlatform.o bitstream.o mask.o mmask.o mqrspec.o qrencode.o qrinput.o qrspec.o rscode.o split.o
headers = bitstream.h lcdgraphics.h mask.h mmask.h mqrspec.h platform.h qrencode.h qrencode_inner.h qrinput.h qrspec.h rscode.h split.h

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

all : $(binary) $(signature) VERIFONE.ZIP

$(binary) : $(objects)
	$(VRXSDK)\bin\vrxcc -g $(objects) $(Libs) -o $(binary)
#	Important Stack and Heap sizes, TCP/IP and SSL need them in order to work fine and with Threads.
#   REASON for a small stack size - this sets the stack for the main (UI) thread.
#   The TCP/IP library will be run entirely by the 2nd thread, whose stack is (I believe) taken from the heap when created.
#   TCP/IP with SSL on Vx570 needs 120KB heap and 27KB stack (which will come off the heap at run_thread).
#   So UI thread can have 24 KB stack.  COMM thread can have 48KB stack and 120KB heap, total 168KB heap, plus a little margin.
	C:\VerixVAps\VRXSDK\bin\vrxhdr -s 32000 -h 48000 $(binary)

$(signature): $(binary) vfbtc.fst certif.crt
	"%VSFSTOOL%\filesignature.exe" "vfbtc.fst" -nogui
	copy $(binary).p7s $(signature) /Y
	del $(binary).p7s

	
###### Compile #######
# The -g option of vrxcc is to enable debugging symbols and output.  Remove it for
# normal usage.

$(objects): %.o: %.c $(headers)
	$(VRXSDK)\bin\vrxcc -c $(Includes) $(COptions) $<
	
###### Make VERIFONE.ZIP file for USB-stick installation	

VERIFONE.ZIP : $(binary) $(signature) config.sys cards.vft gprs.res
	if not exist F1 md F1
	copy $(binary) F1 /y
	copy $(signature) F1 /y
	copy config.sys config.$$$$$$ /y
	del VERIFONE.ZIP
	zip -add VERIFONE.ZIP -path F1\$(binary) F1\$(signature) cards.vft config.$$$$$$ gprs.res

config.sys : config.txt
	$(VRXSDK)\bin\vlr -c config.txt config.sys
	
clean:
	del *.o
	del *.axf
	del config.$$$$$$
	del VERIFONE.ZIP
	del $(binary)
	del $(signature)