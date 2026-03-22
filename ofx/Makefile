CXXFLAGS = -fvisibility=hidden -IOFXInc -IOFXSupportInc
ARCH_FLAGS = -arch arm64 -arch x86_64
CXXFLAGS += $(ARCH_FLAGS)
LDFLAGS = -bundle -fvisibility=hidden -framework Metal -framework AppKit -framework OpenGL
LDFLAGS += $(ARCH_FLAGS)

BUNDLE_DIR = PrimeraOFX.ofx.bundle/Contents/MacOS/

SUPPORT_OBJS = ofxsCore.o ofxsImageEffect.o ofxsInteract.o ofxsLog.o \
               ofxsMultiThread.o ofxsParams.o ofxsProperty.o ofxsPropertyValidation.o

PrimeraOFX.ofx: PrimeraPlugin.o MetalKernel.o $(SUPPORT_OBJS)
	$(CXX) $^ -o $@ $(LDFLAGS)
	mkdir -p $(BUNDLE_DIR)
	cp PrimeraOFX.ofx $(BUNDLE_DIR)
	cp Info.plist PrimeraOFX.ofx.bundle/Contents/

MetalKernel.o: MetalKernel.mm
	$(CXX) -c $< $(CXXFLAGS)

%.o: OFXSupport/%.cpp
	$(CXX) -c $< $(CXXFLAGS)

clean:
	rm -f *.o *.ofx
	rm -fr PrimeraOFX.ofx.bundle

install: PrimeraOFX.ofx
	cp -fr PrimeraOFX.ofx.bundle /Library/OFX/Plugins

dev: clean PrimeraOFX.ofx install
	@echo "--- Installed. Restart Resolve to load. ---"

.PHONY: clean install dev
