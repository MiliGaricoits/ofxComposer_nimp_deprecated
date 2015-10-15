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
    //
    ofAddListener(ofEvents().mouseMoved, this, &ofxComposer::_mouseMoved);
	ofAddListener(ofEvents().mousePressed, this, &ofxComposer::_mousePressed);
	ofAddListener(ofEvents().mouseReleased, this, &ofxComposer::_mouseReleased);
	ofAddListener(ofEvents().keyPressed, this, &ofxComposer::_keyPressed);
    ofAddListener(ofEvents().windowResized, this, &ofxComposer::_windowResized);
    // nico SrollBar
    ofAddListener(ofEvents().mouseDragged, this, &ofxComposer::_mouseDragged);
    
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
            ofxPatch *nPatch = new ofxPatch();
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
    
    for(map<int,ofxPatch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
        it->second->saveSettings(configFile);
    }
}

bool ofxComposer::addPatchFromFile(string _filePath, ofPoint _position){
    bool loaded = false;
    
    ofxPatch *nPatch = new ofxPatch();
    loaded = nPatch->loadFile( _filePath, "config.xml" );
    
    if ( loaded ){
        nPatch->move( _position );
        nPatch->scale(0.5);
        nPatch->saveSettings();
        ofAddListener( nPatch->title->close , this, &ofxComposer::closePatch);
        patches[nPatch->getId()] = nPatch;
    }
    
    return loaded;
}

bool ofxComposer::addPatchWithOutFile(string _type, ofPoint _position){
    bool loaded = false;
    
    ofxPatch *nPatch = new ofxPatch();
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
        for(map<int,ofxPatch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
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

// nico ScrollBar
//void ofxComposer::setup(){
    /*
     The "panel" is a frame. This frame contains the displayed images, and the scroll bar.
     The scroll bar contains a "grip". The user can drag the grip with the mouse.
     */
    
    /*gap = 10;               // Distance between rectangles, and between rectangles and scroll bar
    margin = 20;            // Distance between the edge of the screen and the panel frame
    scrollBarWidth = 20;
    
    // Now two rectangles, for the scroll bar and his grip placements
    // Coordinates are relative to the panel coordinates, not to the screen coordinates
    // This is a first initialisation, but we don't know many things about these placements at this state
    scrollBarRectangle = ofRectangle(0, 0, scrollBarWidth, 0);
    gripRectangle = ofRectangle(0, 0, scrollBarWidth, 0);
    
    isDraggingGrip = false; // true when the user is moving the grip
    isMouseOverGrip = false; // true when the mouse is over the grip
}*/

//-------------------------------------------------------------- LOOP
void ofxComposer::update(){
    for(map<int,ofxPatch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
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
    
    //nico scrollBar
    //updateScrollBar();
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
    
    //nico ScrollBar begin
    //ofTranslate(0, -contentScrollY);
    //nico scrollBar end
    
    //  Draw Patches
    //
    for(map<int,ofxPatch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
        it->second->draw();
    }
    
    //  Draw active line
    //
    if (selectedDot >= 0){
        ofLine(patches[selectedDot]->getOutPutPosition(), ofPoint(ofGetMouseX(),ofGetMouseY()));
    }
    
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
    
    
    
    // nico ScrollBar begin
    // Add a translation to bring the panel to the good position
   /* ofPushMatrix();
        ofTranslate(margin, margin, 0);
    // Draw the scroll bar, is needed
        if (isScrollBarVisible) {
            ofSetColor(110);
            ofRect(scrollBarRectangle);
            if (isDraggingGrip || isMouseOverGrip) {
                ofSetColor(230);
            } else {
                ofSetColor(180);
            }
            ofRect(gripRectangle);
        }
    
    // Remove the translation added at the begining
    ofPopMatrix();*/
    // ScrollBar end
    
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
    
    for(map<int,ofxPatch*>::reverse_iterator rit = patches.rbegin(); rit != patches.rend(); rit++ ){
        if (rit->second->isOver(mouse)){
            activePatch( rit->first );
            break;
        }
    }
    
    // nico scrollBar begin
    /*if (isScrollBarVisible) {
        ofRectangle r = gripRectangle;
        r.translate(margin, margin); // This translation because the coordinates of the grip are relative to the panel, but the mouse position is relative to the screen
        isMouseOverGrip = r.inside(e.x, e.y);
    } else {
        isMouseOverGrip = false;
    }*/
    // nico ScrollBar end
    
}

void ofxComposer::activePatch( int _nID ){
    if ( (_nID != -1) && (patches[_nID] != NULL) ){
        selectedID = _nID;
        
        for(map<int,ofxPatch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
            if (it->first == _nID)
                it->second->bActive = true;
            else
                it->second->bActive = false;
        }
    }
}

void ofxComposer::_mousePressed(ofMouseEventArgs &e){
    ofVec2f mouse = ofVec2f(e.x, e.y);

    selectedDot = -1;    
    for(map<int,ofxPatch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
        if ( (it->second->getOutPutPosition().distance(mouse) < 5) && (it->second->bEditMode) && !(it->second->bEditMask) ){
            selectedDot = it->first;
            it->second->bActive = false;
            selectedID = -1;
        }
    }
    
    if (selectedDot == -1){
        for(map<int,ofxPatch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
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
    
    // nico zoom/drag
    if(!isAnyPatchHit(e.x, e.y)){
        disabledPatches = true;
    }
    
    //nico Scrollbar begin
    // Check if the click occur on the grip
    /*if (isScrollBarVisible) {
        ofRectangle r = gripRectangle;
        r.translate(margin, margin); // This translation because the coordinates of the grip are relative to the panel, but the mouse position is relative to the screen
        if (r.inside(e.x, e.y)) {
            isDraggingGrip = true;
            mousePreviousY = e.y;
        }
    }*/
    // scroll bar end
}


// Nico ZOOM & DRAG
void ofxComposer::_mouseDragged(ofMouseEventArgs &e){
    ofVec3f mouse = ofVec3f(e.x, e.y,0);
    ofVec3f mouseLast = ofVec3f(ofGetPreviousMouseX(),ofGetPreviousMouseY(),0);
    
    // si el mouse esta siendo arrastrado y no hay un nodo abajo
    if(disabledPatches){
        // si apreto el boton izquierdo muevo todos los nodos
        if(e.button == 0){
            for(map<int,ofxPatch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
                it->second->moveDiff(mouse-mouseLast);
            }
        }
        
        // si apreto el boton derecho, hago zoom in/out
        if(e.button == 2){
            float scaleDiff = (mouse.y - mouseLast.y)*ZOOM_SENSITIVITY;
            float scale = ZOOM_UNIT + scaleDiff;
            for(map<int,ofxPatch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
                it->second->scale(scale);
            }
        }
    }
    
}

void ofxComposer::_mouseReleased(ofMouseEventArgs &e){
    ofVec2f mouse = ofVec2f(e.x, e.y);
    
    if (selectedDot != -1){
        for(map<int,ofxPatch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
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
    
    // nico ScrollBar
    //isDraggingGrip = false;
    
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
    
    // nico scrollBar
    // Place the grip at the top of the scroll bar if the size of the panel change
    //gripRectangle.y = 0;
}



// Nico Zoom
bool ofxComposer::isAnyPatchHit(float x, float y){
    ofPoint *point = new ofPoint(x,y);
    bool isAnyHit = false;
    for(map<int,ofxPatch*>::iterator it = patches.begin(); (it != patches.end()) && !isAnyHit ; it++ ){
        if(it->second->isOver(*point)){
            isAnyHit = true;
        }
    }
    delete point;
    return isAnyHit;
}






// nico Scrollbar
/*void ofxComposer::_mouseDragged(ofMouseEventArgs &e){
    if (isScrollBarVisible && isDraggingGrip) {
        // Move the grip according to the mouse displacement
        int dy = e.y - mousePreviousY;
        mousePreviousY = e.y;
        gripRectangle.y += dy;
        
        // Check if the grip is still in the scroll bar
        if (gripRectangle.y < 0) {
            gripRectangle.y = 0;
        }
        if (gripRectangle.getBottom() > scrollBarRectangle.getBottom()) {
            gripRectangle.y = scrollBarRectangle.getBottom() - gripRectangle.height;
        }
    }
}*/

//nico scrollbar begin
/*void ofxComposer::updateScrollBar(){
    // The size of the panel. All the screen except margins
    panelWidth = ofGetWidth() - margin * 2;
    panelHeight = ofGetHeight() - margin * 2;
    
    // Space available for displayed rectangles
    int availableWidth = panelWidth - scrollBarWidth - gap;
    
    // Coordinates for first rectangle
    int x = 0;
    int y = 0;
    
//    ofRectangle * r;
    
    // Place the rectangles in rows and columns. A row must be smaller than availableWidth
    // After this loop, we know that the rectangles fits the panel width. But the available panel height can be to small to display them all.
    //for (int i = 0; i < images.size(); ++i) {
    int coordMasBaja = 0;
    for(map<int,ofxPatch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
        if(coordMasBaja < it->second->getPos().y){
            coordMasBaja = it->second->getPos().y;
        }
    }
    
    
    gripRectangle.x = scrollBarRectangle.x; // Also adjust the grip x coordinate
    
    int contentHeight = coordMasBaja; // Total height for all the rectangles
    // TODO: take care if colors.size() == 0
    
    if (contentHeight > panelHeight) {
        // In the case where there's not enough room to display all the rectangles
        
        // First, the scroll bar
        
        // Show the scroll bar
        isScrollBarVisible = true;
        // Set the scroll bar height to fit the panel height
        scrollBarRectangle.height = panelHeight;
        
        
        // Now, the grip
        
        // This ratio is between 0 and 1. The smaller it is, the smaller the grip must be.
        // If its value is 0.5, for example, it means that there's only half of the room in the panel to display all the rectangles.
        float gripSizeRatio = (float)panelHeight / (float)contentHeight;
        
        // Compute the height of the grip, according to this ratio
        gripRectangle.height = panelHeight * gripSizeRatio;
        
        // Now, the vertical scrolling to add to the rectangles position
        
        // this ratio, between 0 and 1, tell us the amount of scrolling to add if the grip is at the bottom of the scroll bar
        float scrollMaxRatio = 1 - gripSizeRatio;
        
        // this ration tell us how much the grip is down. If 0, the grip is at the top of the scroll bar.
        // if 1, the grip is at the bottom of the scroll bar
        float gripYRatio = gripRectangle.y / (scrollBarRectangle.height - gripRectangle.height);
        
        // Now, the amount of scrolling to do, according to the twos previous ratios
        float contentScrollRatio = gripYRatio * scrollMaxRatio;
        
        // And now the scrolling value to add to each rectangle y coordinate
        contentScrollY = contentHeight * contentScrollRatio;
        
    } else {
        // In the case where there's enough room to display all the rectangles
        
        isScrollBarVisible = false;
        contentScrollY = 0;
        
    }
}*/
// nico scrollbar end