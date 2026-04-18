CC=clang-cl
CXXFLAGS=/winsdkdir /opt/winsdk/sdk /vctoolsdir /opt/winsdk/crt /c /I"/includes" /W4 /WX /diagnostics:column /O1 /Ob2 /Oi /Os /Oy /D NDEBUG /D _UNICODE /D UNICODE /GF- /MT /Zp8 /GS- /Gy- /fp:precise /fp:except- /guard:ehcont- /GR- /std:c++20 /FA /Fa"/output/" /Fo"/output/" /Gd /TP /showIncludes /Zl --target=amd64-pc-windows-msvc  -v 

SRC=/src
OUTPUT=/output
TARGET=psi

all: $(TARGET)

$(TARGET): 
	$(CC) $(CXXFLAGS) $(SRC)/$(TARGET).cc 	
	@cp $(SRC)/$(TARGET).cna $(OUTPUT)
	@python3 /app/boflint.py --loader cs $(OUTPUT)/$(TARGET).obj --logformat vs --nocolor > /output/$(TARGET).lint
