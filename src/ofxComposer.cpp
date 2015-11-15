//
//  ofxComposer.cpp
//  GPUBurner
//
//  Created by Patricio Gonzalez Vivo on 3/9/12.
//  Copyright (c) 2012 http://www.PatricioGonzalezVivo.com All rights reserved.
//

#include "ofxComposer.h"

//  HELP screen -> F1
//
string helpScreen = "\n \
    | ofxComposer help\n \
    ---------------------------------------------------\n \
    \n \
    - F1:   Turn ON/OFF this help message\n \
    - F2:   Surface Edit-Mode on/off\n \
    - F3:   Masking-Mode ON/OFF (need Edit-Mode ) \n \
    \n \
            On mask mode on:\n \
                            - x: delete mask path point\n \
                            - r: reset mask path\n \
    \n \
    - F4:   Reset surface coorners\n \
    - F5:   Add ofxGLEditor (temporal!!!) and if have it add ofVideoGrabber (temporal!!!)\n \
    - F6:   Add ofShader (temporal!!!)\n \
    - F7:   Turn ON/OFF the fullscreen-mode\n \
    \n \
    Mouse and Coorners: \n \
    - Left Button Drag:     coorner proportional scale \n \
    - Left Button Drag + R: Rotate Patch\n \
    - Middle Button Drag \n \
            or \n \
      Right Drag + A:       centered proportional scale\n \
    - Right Button Drag:    coorner transformation\n ";
    
ofxComposer::ofxComposer(){
    
    //  Event listeners
    //  defined in derived class
    
#ifdef USE_OFXGLEDITOR       
    editor.setup("menlo.ttf");
    editor.setCurrentEditor(1);
    editorBgColor.set(0,0);
    editorFgColor.set(0,0);
    editorFbo.allocate(ofGetWindowWidth(), ofGetWindowHeight());
    editorFbo.begin();
    ofClear(editorBgColor);
    editorFbo.end();
#endif
    
    //  Default parameters
    //
    configFile = "config.xml";
    selectedDot = -1;
    selectedID = -1;
    bEditMode = true;
    bGLEditorPatch = false;
    bHelp = false;
    
    // nico zoom/drag
    disabledPatches = false;
    
    // mili  nodes aligned
    verticalAlign1 = 0;
    verticalAlign2 = 0;
    horizontalAlign1 = 0;
    horizontalAlign2 = 0;
    
    // nico multipleSelect
    multipleSelectFromX = 0;
    multipleSelectFromY = 0;
}

void ofxComposer::load(string _fileConfig){
    if (_fileConfig != "default")
        configFile = _fileConfig;
    
    ofxXmlSettings XML;
    
    patches.clear();
    if (XML.loadFile(_fileConfig)){
        
#ifdef USE_OFXGLEDITOR       
        editor.setup(XML.getValue("general:console:font", "menlo.ttf"));
#endif
        int totalPatchs = XML.getNumTags("surface");
                
        // Load each surface present on the xml file
        //
        for(int i = 0; i < totalPatchs ; i++){
            patch *nPatch = new patch();
            bool loaded = nPatch->loadSettings(i, "config.xml");
            
            if (loaded){

#ifdef USE_OFXGLEDITOR
                if (nPatch->getType() == "ofxGLEditor"){
                    ofLog(OF_LOG_NOTICE,"ofxComposer: ofxGLEditor loaded");
                    nPatch->setTexture( editorFbo.getTextureReference(), 0);
                    bGLEditorPatch = true;
                }
#endif
                // Listen to close bottom on the titleBar
                //
                ofAddListener( nPatch->title->close , this, &ofxComposer::closePatch);
                
                // Insert the new patch into the map
                //
                patches[nPatch->getId()] = nPatch;
                
                //mili
                nPatch->setMainCanvas(this->gui);
                //
            }
        }
        
        // Load links between Patchs
        //
        for(int i = 0; i < totalPatchs ; i++){
            if (XML.pushTag("surface", i)){
                int fromID = XML.getValue("id", -1);
                
                if (XML.pushTag("out")){
    
                    int totalLinks = XML.getNumTags("dot");
                    for(int j = 0; j < totalLinks ; j++){
                        
                        if (XML.pushTag("dot",j)){
                            int toID = XML.getValue("to", 0);
                            int nTex = XML.getValue("tex", 0);
                            
                            // If everything goes ok "i" will match the position of the vector
                            // with the position on the XML, in the same place of the vector array
                            // defined on the previus loop
                            //
                            connect( fromID, toID, nTex);
                            
                            XML.popTag();
                        }
                    }
                    XML.popTag();
                }
                XML.popTag();
            }
        }
    }
}

void ofxComposer::save(string _fileConfig ){
    if (_fileConfig != "default"){
        configFile = _fileConfig;
    }
    
    for(map<int,patch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
        it->second->saveSettings(configFile);
    }
}

bool ofxComposer::addPatchFromFile(string _filePath, ofPoint _position){
    bool loaded = false;
    
    patch *nPatch = new patch();
    loaded = nPatch->loadFile( _filePath, "config.xml" );
    
    if ( loaded ){
        nPatch->move( _position );
        nPatch->scale(0.5);
        nPatch->saveSettings();
        ofAddListener( nPatch->title->close , this, &ofxComposer::closePatch);
        patches[nPatch->getId()] = nPatch;
        
        //mili
        nPatch->setMainCanvas(this->gui);
        //
    }
    
    return loaded;
}

bool ofxComposer::addPatchWithOutFile(string _type, ofPoint _position){
    bool loaded = false;
    
    patch *nPatch = new patch();
    loaded = nPatch->loadType( _type, "config.xml" );
    
    if ( loaded ){
        //mili (scale antes de move)
        nPatch->scale(0.5);
        nPatch->move( _position );
        //
        nPatch->saveSettings();
        ofAddListener( nPatch->title->close , this, &ofxComposer::closePatch);
#ifdef USE_OFXGLEDITOR
        if (nPatch->getType() == "ofxGLEditor"){
            nPatch->setTexture( editorFbo.getTextureReference(), 0);
        }
#endif
        
        patches[nPatch->getId()] = nPatch;
        //mili
        nPatch->setMainCanvas(this->gui);
        //
    }
    
    return loaded;
}

bool ofxComposer::connect( int _fromID, int _toID, int nTexture ){
    bool connected = false;
    
    if ((_fromID != -1) && (patches[_fromID] != NULL) && 
        (_toID != -1) && (patches[_toID] != NULL) && 
        (patches[ _toID ]->getType() == "ofShader") ) {
        
        LinkDot newDot;
        newDot.pos = patches[ _fromID ]->getOutPutPosition();
        newDot.toId = patches[ _toID ]->getId();
        newDot.to = &(patches[ _toID ]->inPut[ nTexture ]);
        newDot.toShader = patches[ _toID ]->getShader();
        newDot.nTex = nTexture;
        
        patches[ _fromID ]->outPut.push_back( newDot );
        connected = true;
    }
    
    return connected;
}

void ofxComposer::closePatch( int &_nID ){
    bool deleted = false;
         
    if ( (_nID != -1) && (patches[_nID] != NULL) ){
        int targetTag = 0;
        
        if (patches[_nID]->getType() == "ofxGLEditor")
            bGLEditorPatch = false;
        
        // Delete links Dependences
        //
        for(map<int,patch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
            for (int j = it->second->outPut.size()-1; j >= 0 ; j--){
                if ( it->second->outPut[j].toId == _nID){
                    it->second->outPut.erase( it->second->outPut.begin() + j );
                    it->second->saveSettings();
                }
            }
        }
        
        // Delete object from memory and then from vector 
        //
        selectedID = -1;
        delete &patches[_nID];
        patches.erase(_nID);
        
        // Delete XML Data
        //
        ofxXmlSettings XML;
        if ( XML.loadFile( configFile ) ){
            int totalSurfaces = XML.getNumTags("surface");
            for (int i = 0; i < totalSurfaces; i++){
                if (XML.pushTag("surface", i)){
                    if ( XML.getValue("id", -1) == _nID){
                        targetTag = i;
                    }
                    XML.popTag();
                }
            }
            
            XML.removeTag("surface", targetTag);
            XML.saveFile();
        }
    }
}

//-------------------------------------------------------------- LOOP
void ofxComposer::update(){
    for(map<int,patch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
        it->second->update();
    }
    
    if ( (bEditMode) && (selectedID >= 0)){
#ifdef USE_OFXGLEDITOR
        if (patches[selectedID]->getType() == "ofShader"){
            editorBgColor.lerp(ofColor(0,150), 0.01);
            editorFgColor.lerp(ofColor(255,255), 0.1);
        } else {
            editorBgColor.lerp(ofColor(0,0), 0.01);
            editorFgColor.lerp(ofColor(0,0), 0.05);
        }
        
        editorFbo.begin();
        //ofEnableAlphaBlending();
        ofClear(editorBgColor);
        ofDisableBlendMode();
        ofRotate(180, 0, 1, 0);
        ofSetColor(255,255);
        editor.draw();
        editorFbo.end();
#endif
    }
    
}


void ofxComposer::draw(){
    ofPushView();
    ofPushStyle();
    ofPushMatrix();

    ofEnableAlphaBlending();
    
#ifdef USE_OFXGLEDITOR
    //  Draw the GLEditor if it�s not inside a Patch
    //
    if (bEditMode && !bGLEditorPatch){
        ofPushMatrix();
        ofRotate(180, 1, 0, 0);
        ofTranslate(0, -ofGetWindowHeight());
        ofSetColor(editorFgColor);
        editorFbo.draw(0, 0);
        ofPopMatrix();
    }
#endif
    
    //  Draw Patches
    //
    for(map<int,patch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
        it->second->customDraw();
    }
    
    //  Draw active line
    //
    if (selectedDot >= 0){
        ofLine(patches[selectedDot]->getOutPutPosition(), ofPoint(ofGetMouseX(),ofGetMouseY()));
    }
    
    //mili - nodes aligned
    if (verticalAlign1) {
        ofSetColor(255, 208, 111);
        ofLine(verticalAlign1, 0, verticalAlign1, ofGetHeight());
    }
    if (verticalAlign2) {
        ofSetColor(255, 208, 111);
        ofLine(verticalAlign2, 0, verticalAlign2, ofGetHeight());
    }
    if (verticalAlign3) {
        ofSetColor(255, 208, 111);
        ofLine(verticalAlign3, 0, verticalAlign3, ofGetHeight());
    }
    if (horizontalAlign1) {
        ofSetColor(255, 208, 111);
        ofLine(0, horizontalAlign1, ofGetWidth(), horizontalAlign1);
    }
    if (horizontalAlign2) {
        ofSetColor(255, 208, 111);
        ofLine(0, horizontalAlign2, ofGetWidth(), horizontalAlign2);
    }
    if (horizontalAlign3) {
        ofSetColor(255, 208, 111);
        ofLine(0, horizontalAlign3, ofGetWidth(), horizontalAlign3);
    }
    //
        
    //  Draw Help screen
    //
    if (bHelp){
        ofSetColor(255);
        ofDrawBitmapString(helpScreen, 20, ofGetWindowHeight()*0.5- 11.0*15.0);
    }
    
    ofDisableBlendMode();
    ofEnableAlphaBlending();

    ofPopMatrix();
    ofPopStyle();
    ofPopView();
    
    
    // nico multipleSelect
    ofPushMatrix();
        ofNoFill();
        ofRect(multipleSelectRectangle);
    ofPopMatrix();
    
}

//-------------------------------------------------------------- EVENTS
void ofxComposer::_keyPressed(ofKeyEventArgs &e){
    if (e.key == OF_KEY_F1 ){
        bHelp = !bHelp;
    } else if (e.key == OF_KEY_F2 ){
        bEditMode = !bEditMode;
    } else if ((e.key == OF_KEY_F3 ) || (e.key == OF_KEY_F4 ) ){
        //  Special keys reserved for Patch Events
        //
    } else if (e.key == OF_KEY_F5 ){
        
        if ( bGLEditorPatch )
            addPatchWithOutFile("ofVideoGrabber", ofPoint(ofGetMouseX(),ofGetMouseY()));
        else
            bGLEditorPatch = addPatchWithOutFile("ofxGLEditor", ofPoint(ofGetMouseX(),ofGetMouseY()));
        
    } else if ( e.key == OF_KEY_F6 ){
        addPatchWithOutFile("ofShader", ofPoint(ofGetMouseX(),ofGetMouseY()));

    } else if (e.key == OF_KEY_F7){
        ofToggleFullscreen();

#ifdef USE_OFXGLEDITOR
        editor.reShape();
        editorFbo.allocate(ofGetWindowWidth(),ofGetWindowHeight());
        editorFbo.begin();
        ofClear(editorBgColor);
        editorFbo.end();
#endif
    } else {
        //  If no special key was pressed and the GLEditor is present pass the key
        //
#ifdef USE_OFXGLEDITOR
        editor.keyPressed(e.key);
        
        if (selectedID >= 0){
            if (patches[selectedID]->getType() == "ofShader"){
                patches[selectedID]->setFrag(editor.getText(1));
                patches[selectedID]->saveSettings();
            }
        }
#endif
        
    }
}

void ofxComposer::_mouseMoved(ofMouseEventArgs &e){
    ofVec2f mouse = ofVec2f(e.x, e.y);
    
//    for(map<int,ofxPatch*>::reverse_iterator rit = patches.rbegin(); rit != patches.rend(); rit++ ){
//        if (rit->second->isOver(mouse)){
//            activePatch( rit->first );
//            break;
//        }
//    }
}

void ofxComposer::activePatch( int _nID ){
    if ( (_nID != -1) && (patches[_nID] != NULL) ){
        selectedID = _nID;
        
        for(map<int,patch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
            if (it->first == _nID)
                it->second->bActive = true;
            else
                it->second->bActive = false;
        }
    }
}

void ofxComposer::_mousePressed(ofMouseEventArgs &e){
    ofVec2f mouse = ofVec2f(e.x, e.y);
    
    // si no estoy clickeando sobre ninguna de las 2 scrollbars, veo que hago
    // si estoy clickeando una de las scrollbars, no tengo que hacer nada aca
    if(!draggingGrip && !draggingHGrip) {
        int idPatchHit = isAnyPatchHit(e.x, e.y);
        // nico zoom/drag
        if(idPatchHit == -1){
            disabledPatches = true;
            deactivateAllPatches();
        }else{
            disabledPatches = false;
            for(map<int,patch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
                if(!patches.find(idPatchHit)->second->bActive){
                    activePatch(idPatchHit);
                    break;
                }
            }
        }
        
        selectedDot = -1;
        for(map<int,patch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
            if ( (it->second->getOutPutPosition().distance(mouse) < 5) && (it->second->bEditMode) && !(it->second->bEditMask) ){
                selectedDot = it->first;
                it->second->bActive = false;
                selectedID = -1;
            }
            
        }
        
        if (selectedDot == -1){
            for(map<int,patch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
                if ((it->second->bActive) && (it->second->bEditMode) && !(it->second->bEditMask)){
                    selectedID = it->first;
#ifdef USE_OFXGLEDITOR
                    //if (bGLEditorPatch
                    if ((it->second->getType() == "ofShader")){
                        editor.setText(it->second->getFrag(), 1);
                    }
#endif
                }
            }
        }
        
        // nico multipleSelect
        if(disabledPatches && e.button == 0 && !gui->getOtherSelected()){
            multipleSelectFromX = e.x;
            multipleSelectFromY = e.y;
            multipleSelectRectangle.x = e.x;
            multipleSelectRectangle.y = e.y;
        }
    }
    
}


// Nico ZOOM & DRAG
void ofxComposer::_mouseDragged(ofMouseEventArgs &e){
    ofVec3f mouse = ofVec3f(e.x, e.y,0);
    ofVec3f mouseLast = ofVec3f(ofGetPreviousMouseX(),ofGetPreviousMouseY(),0);
    
    // si el mouse esta siendo arrastrado y no hay un nodo/link/nodeInput abajo
    if ( disabledPatches && !draggingGrip && !draggingHGrip && (!isAnyLinkHit()) && (!gui->getOtherSelected()) ) {
        // si apreto el boton izquierdo muevo todos los nodos
//        if(e.button == 0){
//            movePatches(mouse - mouseLast);
//        }
        
        // si es el boton izquierdo hago multiple selection
        if(e.button == 0){
            multipleSelectRectangle.width = e.x - multipleSelectFromX;
            multipleSelectRectangle.height = e.y - multipleSelectFromY;
        }
        
        // si apreto el boton derecho, hago zoom in/out
        if(e.button == 2){
            scalePatches(mouse.y - mouseLast.y);
        }
    } else {
        int activePatch = isAnyPatchHit(mouse.x, mouse.y);
        if (activePatch != -1) {
        
            patch* p = patches[activePatch];
            verticalAlign1 = 0;
            verticalAlign2 = 0;
            verticalAlign3 = 0;
            horizontalAlign1 = 0;
            horizontalAlign2 = 0;
            horizontalAlign3 = 0;
            
            for(map<int,patch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
                
                if (it->second != p) {
                    if ((int)it->second->getCoorners()[0].x == (int)p->getCoorners()[0].x or
                        (int)it->second->getCoorners()[1].x == (int)p->getCoorners()[0].x) {
                        verticalAlign1 = p->getCoorners()[0].x ;
                    }
                    if ((int)(it->second->getCoorners()[0].x + it->second->getBox().width/2) == (int)(p->getCoorners()[0].x + p->getBox().width/2)) {
                        verticalAlign2 = (p->getCoorners()[0].x + p->getBox().width/2);
                    }
                    if ((int)it->second->getCoorners()[0].x == (int)p->getCoorners()[1].x or
                        (int)it->second->getCoorners()[1].x == (int)p->getCoorners()[1].x ) {
                        verticalAlign3 = p->getCoorners()[1].x;
                    }
                    
                    if ((int)it->second->getCoorners()[1].y == (int)p->getCoorners()[1].y or
                        (int)it->second->getCoorners()[3].y == (int)p->getCoorners()[1].y) {
                        horizontalAlign1 = p->getCoorners()[1].y ;
                    }
                    if ((int)(it->second->getCoorners()[1].y + it->second->getBox().height/2) == (int)(p->getCoorners()[1].y + p->getBox().height/2)) {
                        horizontalAlign2 = (p->getCoorners()[1].y + p->getBox().height/2);
                    }
                    if ((int)it->second->getCoorners()[1].y == (int)p->getCoorners()[3].y or
                        (int)it->second->getCoorners()[3].y == (int)p->getCoorners()[3].y ) {
                        horizontalAlign3 = p->getCoorners()[3].y;
                    }
                }
            }
        }
    }
}

void ofxComposer::_mouseReleased(ofMouseEventArgs &e){
    ofVec2f mouse = ofVec2f(e.x, e.y);
    
    if (selectedDot != -1){
        for(map<int,patch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
            if ((selectedDot != it->first) &&                   // If not him self
                (it->second->getType() == "ofShader") &&   // The target it´s a shader
                (it->second->bEditMode) &&               // And we are in editMode and not on maskMode
                !(it->second->bEditMask) ){
                
                for (int j = 0; j < it->second->inPut.size(); j++){
                    
                    // And after checking in each dot of each shader...
                    // ... fin the one where the mouse it´s over
                    //
                    if ( it->second->inPut[j].pos.distance(mouse) < 5){
                        
                        // Once he founds it
                        // make the link and forget the selection
                        //
                        connect( selectedDot , it->first, j );
                        patches[ selectedDot ]->saveSettings();
                        selectedDot = -1;
                    }
                }
            }
            
            //nico zoom/drag
            it->second->setDisablePatch(false);
        }
        
        // If he release the mouse over nothing it will clear all
        // the connections of that dot.
        //
        if (selectedDot != -1){
            patches[selectedDot]->outPut.clear();
            patches[selectedDot]->saveSettings();
            selectedDot = -1;
        }
    }
    
    //mili - aligned nodes
    verticalAlign1 = 0;
    verticalAlign2 = 0;
    verticalAlign3 = 0;
    horizontalAlign1 = 0;
    horizontalAlign2 = 0;
    horizontalAlign3 = 0;
    //
    
    // nico multipleSelect
    multipleSelectAndReset();
    
    // nico zoom/drag
    disabledPatches = false;
}

void ofxComposer::_windowResized(ofResizeEventArgs &e){
#ifdef USE_OFXGLEDITOR
    editor.reShape();
    editorFbo.allocate(e.width, e.height);
    editorFbo.begin();
    ofClear(editorBgColor);
    editorFbo.end();
#endif
}



// Nico Zoom
int ofxComposer::isAnyPatchHit(float x, float y){
    ofPoint *point = new ofPoint(x,y);
    int isAnyHit = -1;
    for(map<int,patch*>::reverse_iterator rit = patches.rbegin(); rit != patches.rend(); rit++ ){
        if (rit->second->isOver(*point)){
            isAnyHit = rit->first;
            break;
        }
    }
    delete point;
    return isAnyHit;
}

void ofxComposer::movePatches(ofVec3f diff){
    for(map<int,patch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
        it->second->moveDiff(diff);
    }
}

void ofxComposer::scalePatches(float yDiff){
    float scale = ZOOM_UNIT + yDiff*ZOOM_SENSITIVITY;
    for(map<int,patch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
        it->second->scale(scale);
    }
}

//nico scrollbar begin
int ofxComposer::getPatchesLowestCoord(){
    int coordMasBaja = 1000;
    for(map<int,patch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
        if(coordMasBaja > it->second->getLowestYCoord()){
            coordMasBaja = it->second->getLowestYCoord();
        }
    }
    return coordMasBaja - 30;
}
int ofxComposer::getPatchesHighestCoord(){
    int coordMasAlta = -1;
    for(map<int,patch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
        if(coordMasAlta < it->second->getHighestYCoord()){
            coordMasAlta = it->second->getHighestYCoord();
        }
    }
    return coordMasAlta;
}

int ofxComposer::getPatchesLeftMostCoord(){
    int coordMasIzq = 1000;
    for(map<int,patch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
        if(coordMasIzq > it->second->getLowestXCoord()){
            coordMasIzq = it->second->getLowestXCoord();
        }
    }
    return coordMasIzq - 30;
}
int ofxComposer::getPatchesRightMostCoord(){
    int coordMasDer = -1;
    for(map<int,patch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
        if(coordMasDer < it->second->getHighestXCoord()){
            coordMasDer = it->second->getHighestXCoord();
        }
    }
    return coordMasDer;
}
// nico scrollbar end

void ofxComposer::deactivateAllPatches(){
    for(map<int,patch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
        it->second->bActive = false;
    }
}

// nico multiple select
void ofxComposer::multipleSelectAndReset(){
    if(disabledPatches){
        for(map<int,patch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
            ofRectangle aux = multipleSelectRectangle.getIntersection(it->second->getBox());
            if(aux.getArea() > 0){
                it->second->bActive = true;
            } else{
                it->second->bActive = false;
            }
        }
    }
    
    // reseteo el rectangulo
    multipleSelectFromX = 0;
    multipleSelectFromY = 0;
    multipleSelectRectangle.x = 0;
    multipleSelectRectangle.y = 0;
    multipleSelectRectangle.height = 0;
    multipleSelectRectangle.width = 0;
}

bool ofxComposer::isAnyLinkHit(){
    for(map<int,patch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
        if(it->second->isLinkHit()){
            return true;
        }
    }
    return false;
}
/************************************** GETTERS AND SETTERS ******************************/
bool ofxComposer::isDraggingGrip(){
    return draggingGrip;
}

void ofxComposer::setDraggingGrip(bool dragging){
    draggingGrip = dragging;
}

bool ofxComposer::isDraggingHGrip(){
    return draggingHGrip;
}

void ofxComposer::setDraggingHGrip(bool dragging){
    draggingHGrip = dragging;
}


//mili
void ofxComposer::setMainCanvas(ofxUISuperCanvas* _gui) {
    this->gui = _gui;
}
//