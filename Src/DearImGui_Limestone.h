#define IMGUI_DISABLE_WIN32_DEFAULT_CLIPBOARD_FUNCS
#define IMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCS

#include "imgui\imgui.cpp"

/*
TODO: 
-do stuff with clipping rectangles
-add window height/width info to shader as uniform
*/

static void RenderImGuiVisuals(ImDrawList** const cmd_lists, int cmd_lists_count);
static void InitImGui_LimeStone();

char* ImGuiVertShaderSrc =
"#version 140\n"
"attribute vec2 pos;"
"attribute vec2 uv;"
"attribute vec4 color;"
"uniform vec2 windowDimensions;"
"out vec2 Frag_UV;"
"out vec4 Frag_Color;"
"void main() {"
"    gl_Position = vec4("
"        ( pos.x / ( windowDimensions.x / 2.0 ) ) - 1.0,"
"        -( pos.y / ( windowDimensions.y / 2.0 ) ) + 1.0,"
"        0.001f,"
"        1.0f"
"    );"
"    Frag_UV = uv;"
"    Frag_Color = color;"
"}";

char* ImGuiFragShaderSrc =
"#version 140\n"
"uniform sampler2D texture;"
"in vec2 Frag_UV;"
"in vec4 Frag_Color;"
"void main() {"
"    vec4 texColor = texture2D( texture, Frag_UV.st );"
"    gl_FragColor = Frag_Color * texColor;"
"}";

struct ImguiLimestoneDriver {
    ShaderProgram ImGuiUIShader;
    PtrToGpuMem imguiVertexDataPtr;
    PtrToGpuMem imguiFontTexturePtr;
    ImFontAtlas defaultAtlas;
    ImFont defaultFont;
    RenderDriver* renderDriver;
    float x, y;
};

static void* ImGuiMemAlloc( size_t size ) {

}

static void ImGuiMemFree( void* v) {

}

//COMMENT COPIED FROM IMGUI EXAMPLE
// This is the main rendering function that you have to implement and provide to ImGui (via setting up 'RenderDrawListsFn' in the ImGuiIO structure)
// If text or lines are blurry when integrating ImGui in your engine:
// in your Render function, try translating your projection matrix by (0.5f,0.5f) or (0.375f,0.375f)
static void RenderImGuiVisuals(ImDrawList** const cmd_lists, int cmd_lists_count)
{
    if (cmd_lists_count == 0)
        return;

    ImGuiIO& imguiIO = ImGui::GetIO();
    ImguiLimestoneDriver* imData = (ImguiLimestoneDriver*)imguiIO.UserData;
    RenderDriver* renderDriver = imData->renderDriver;

    #define OFFSETOF(TYPE, ELEMENT) ((size_t)&(((TYPE *)0)->ELEMENT))
    for ( int n = 0; n < cmd_lists_count; n++ )
    {
        const ImDrawList* cmd_list = cmd_lists[n];
        char* vtx_buffer = (char*)&cmd_list->vtx_buffer.front();

        for ( size_t cmd_i = 0; cmd_i < cmd_list->commands.size(); cmd_i++ )
        {
            const ImDrawCmd* pcmd = &cmd_list->commands[cmd_i];

            int posIndex = GetIndexOfShaderInput( 
                &imData->ImGuiUIShader, 
                "pos" 
            );
            int uvIndex = GetIndexOfShaderInput(
                &imData->ImGuiUIShader,
                "uv"
            );
            int colIndex = GetIndexOfShaderInput(
                &imData->ImGuiUIShader,
                "col"
            );
            int windowUniform = GetIndexOfShaderInput(
                &imData->ImGuiUIShader,
                "windowDimensions"
            );

            RenderCommand command = { };
            command.shader = &imData->ImGuiUIShader;
            command.vertexFormat = RenderCommand::INTERLEAVESTREAM;
            command.VertexFormat.bufferForStream = imData->imguiVertexDataPtr;
            command.VertexFormat.vertexAttributeOffsets[ 0 ] = OFFSETOF( ImDrawVert, pos );
            command.VertexFormat.vertexAttributeOffsets[ 1 ] = OFFSETOF( ImDrawVert, uv );
            command.VertexFormat.vertexAttributeOffsets[ 2 ] = OFFSETOF( ImDrawVert, col );
            command.VertexFormat.vertSize = sizeof( ImDrawVert );
            command.VertexFormat.streamData = (void*)vtx_buffer;
            command.VertexFormat.streamSize = pcmd->vtx_count * sizeof( ImDrawVert );

            command.samplerData[ 0 ] = imData->imguiFontTexturePtr;

            float windowDimensionsVec [2] = { imData->x, imData->y };
            command.uniformData[ windowUniform ] = windowDimensionsVec;

            command.elementCount = pcmd->vtx_count;

            renderDriver->Draw( &command, false, true, false, false, true );

            vtx_buffer += ( sizeof( ImDrawVert ) * pcmd->vtx_count );
        }
    }
    #undef OFFSETOF
}

static void UpdateImgui( 
    InputState* i, 
    void* savedInternalState, 
    int windowWidth, 
    int windowHeight 
) {
    ImGui::SetInternalState( savedInternalState );

    float mX = i->mouseX;
    float mY = i->mouseY;

    ImGuiIO& imguiIO = ImGui::GetIO();

    //We reset these functions every frame, to support code hot-reloading
    imguiIO.RenderDrawListsFn = RenderImGuiVisuals;
    //imguiIO.MemAllocFn = ImGuiMemAlloc;
    //imguiIO.MemFreeFn = ImGuiMemFree;

    imguiIO.MousePos.x = ( ( mX + 1.0f ) / 2.0f ) * windowWidth;
    imguiIO.MousePos.y = ( 1.0f - ( ( mY + 1.0f ) / 2.0f ) ) * windowHeight;
    imguiIO.MouseDown[0] = i->mouseButtons[0];

    char* keysInputted = i->keysPressedSinceLastUpdate;
    //24 here is a magic number corresponding to InputState struct's 
    //keysPressedSinceLastUpdate array length
    for( int charIndex = 0; charIndex < 24; ++charIndex ) {
        if( keysInputted[ charIndex ] != 0 ) {
            imguiIO.AddInputCharacter( keysInputted[ charIndex ] );
        }
    }

    imguiIO.KeysDown[ InputState::BACKSPACE ] = i->spcKeys[ InputState::BACKSPACE ];
    imguiIO.KeysDown[ InputState::TAB ] = i->spcKeys[ InputState::TAB ];

    ImguiLimestoneDriver* imData = (ImguiLimestoneDriver*)imguiIO.UserData;

    imData->x = windowWidth;
    imData->y = windowHeight;

    ImGui::NewFrame();
}

static void* InitImGui_LimeStone( 
    Stack* allocater, 
    RenderDriver* rDriver,
    int screenWidth,
    int screenHeight
) {
    ImguiLimestoneDriver* imguiDriver = (ImguiLimestoneDriver*)
        StackAlloc( allocater, sizeof( ImguiLimestoneDriver ) );
    imguiDriver->renderDriver = rDriver;

    size_t imguiStateSize = ImGui::GetInternalStateSize();
    void* imguiState = StackAllocAligned( allocater, imguiStateSize );
    ImGui::SetInternalState( imguiState, true );

    rDriver->CreateShaderProgram(
        ImGuiVertShaderSrc, 
        ImGuiFragShaderSrc, 
        &imguiDriver->ImGuiUIShader 
    );
    imguiDriver->imguiVertexDataPtr = rDriver->AllocNewGpuArray();

    ImGuiIO& imguiIO = ImGui::GetIO();
    imguiIO.DisplaySize.x = screenWidth;
    imguiIO.DisplaySize.y = screenHeight;
    imguiIO.DeltaTime = 1.0f / 60.0f;
    imguiIO.UserData = (void*)imguiDriver;
    imguiIO.KeyMap[ ImGuiKey_Backspace ] = InputState::BACKSPACE;
    imguiIO.KeyMap[ ImGuiKey_Tab ] = InputState::TAB;
    imguiIO.Fonts = &imguiDriver->defaultAtlas;

    unsigned char* pixels;
    int width, height, bytes_per_pixels;
    imguiIO.Fonts->GetTexDataAsRGBA32( 
        &pixels, 
        &width, 
        &height, 
        &bytes_per_pixels
    );
    TextureData imguiTexData = {
        pixels, 
        (uint16)width, 
        (uint16)height,
        bytes_per_pixels
    };
    rDriver->CopyTextureDataToGpuMem( 
        &imguiTexData, 
        &imguiDriver->imguiFontTexturePtr
    );
    imguiIO.Fonts->TexID = (void*)imguiDriver->imguiFontTexturePtr;
    imguiIO.Fonts->ClearInputData();
    imguiIO.Fonts->ClearTexData();

    return imguiState;
}