#define IMGUI_DISABLE_WIN32_DEFAULT_CLIPBOARD_FUNCS
#define IMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCS
#define IMGUI_DISABLE_TEST_WINDOWS
#include "imgui\imgui.cpp"

static void RenderImGuiVisuals(ImDrawList** const cmd_lists, int cmd_lists_count);
static void InitImGui_LimeStone();

char* ImGuiVertShaderSrc =
"#version 140\n"
"attribute vec2 pos;"
"attribute vec2 uv;"
"attribute vec4 color;"
"out vec2 Frag_UV;"
"out vec4 Frag_Color;"
"void main() {"
"    gl_Position = vec4("
"        ( pos.x / 512.0) - 1.0,"
"        -( pos.y/ 340.0) + 1.0,"
"        0.5f,"
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
    Stack* mem;
};

static void* ImGuiMemAlloc( size_t size ) {
    Stack* mem = ((ImguiLimestoneDriver*)ImGui::GetIO().UserData)->mem;
    void* newMem = StackAllocA( mem, size );
    return newMem;
}

static void ImGuiMemFree( void* v) {

}

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

            int posIndex = GetIndexOfProgramVertexInput( 
                &imData->ImGuiUIShader, 
                "pos" 
            );
            int uvIndex = GetIndexOfProgramVertexInput(
                &imData->ImGuiUIShader,
                "uv"
            );
            int colIndex = GetIndexOfProgramVertexInput(
                &imData->ImGuiUIShader,
                "col"
            );
            RenderCommand_Interleaved command = { };
            command.shader = &imData->ImGuiUIShader;
            command.bufferForStream = imData->imguiVertexDataPtr;
            command.vertexAttributeOffsets[ 0 ] = OFFSETOF( ImDrawVert, pos );
            command.vertexAttributeOffsets[ 1 ] = OFFSETOF( ImDrawVert, uv );
            command.vertexAttributeOffsets[ 2 ] = OFFSETOF( ImDrawVert, col );
            command.sampleData[ 0 ] = imData->imguiFontTexturePtr;
            command.vertSize = sizeof( ImDrawVert );

            command.elementCount = pcmd->vtx_count;
            command.streamData = (void*)vtx_buffer;
            command.streamSize = pcmd->vtx_count * sizeof( ImDrawVert );

            renderDriver->DrawInterleavedStream( renderDriver, &command );

            vtx_buffer += ( sizeof( ImDrawVert ) * pcmd->vtx_count );
        }
    }

    #undef OFFSETOF

#if 0

    GL_API* glApi = (GL_API*)drawStruct->internalAPI;

    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glDisable( GL_CULL_FACE );
    glDisable( GL_DEPTH_TEST );
    glDisable( GL_SCISSOR_TEST );

    glUseProgram( drawStruct->ImGuiUIShader.programID );

    int32 im_posAttribute = GetShaderProgramInputPtr( 
        &drawStruct->ImGuiUIShader, 
        "pos" 
    );
    int32 im_uvAttribute = GetShaderProgramInputPtr( 
        &drawStruct->ImGuiUIShader, 
        "uv" 
    );
    int32 im_colorAttribute = GetShaderProgramInputPtr( 
        &drawStruct->ImGuiUIShader, 
        "color" 
    );

    // Setup orthographic projection matrix

    // Render command lists
    #define OFFSETOF(TYPE, ELEMENT) ((size_t)&(((TYPE *)0)->ELEMENT))
    for (int n = 0; n < cmd_lists_count; n++)
    {
        const ImDrawList* cmd_list = cmd_lists[n];
        char* vtx_buffer = (char*)&cmd_list->vtx_buffer.front();

        int startIndex = 0;
        for (size_t cmd_i = 0; cmd_i < cmd_list->commands.size(); cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->commands[cmd_i];

            glBindBuffer( GL_ARRAY_BUFFER, drawStruct->imguiVertexDataPtr );
            glBufferData( GL_ARRAY_BUFFER, pcmd->vtx_count * sizeof(ImDrawVert), vtx_buffer, GL_DYNAMIC_DRAW );

            glEnableVertexAttribArray( im_posAttribute );
            glEnableVertexAttribArray( im_uvAttribute );
            glEnableVertexAttribArray( im_colorAttribute );
            glVertexAttribPointer( im_posAttribute, 2, GL_FLOAT, false, sizeof( ImDrawVert ), (GLvoid*)OFFSETOF( ImDrawVert, pos ) );
            glVertexAttribPointer( im_uvAttribute, 2, GL_FLOAT, true, sizeof( ImDrawVert ), (GLvoid*)OFFSETOF( ImDrawVert, uv ) );
            glVertexAttribPointer( im_colorAttribute, 4, GL_UNSIGNED_BYTE, true, sizeof( ImDrawVert ), (GLvoid*)OFFSETOF( ImDrawVert, col ) );
            glActiveTexture( GL_TEXTURE0 );
            glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->texture_id);

            glDrawArrays( GL_TRIANGLES, 0, (GLsizei)pcmd->vtx_count );
            vtx_buffer += ( sizeof( ImDrawVert ) * pcmd->vtx_count );
        }
    }
    #undef OFFSETOF
    glDisable( GL_BLEND );
    glUseProgram( 0 );
    glEnable( GL_CULL_FACE );
    glEnable( GL_DEPTH_TEST );
    glDisable( GL_SCISSOR_TEST );
#endif
}

static void UpdateImguiInput( InputState* i, void* savedInternalState ) {
    ImGui::SetInternalState( savedInternalState );

    float mX = i->mouseX;
    float mY = i->mouseY;

    ImGuiIO& imguiIO = ImGui::GetIO();

    //We reset these functions every frame, to support code hot-reloading
    imguiIO.RenderDrawListsFn = RenderImGuiVisuals;
    imguiIO.MemAllocFn = ImGuiMemAlloc;
    imguiIO.MemFreeFn = ImGuiMemFree;

    imguiIO.MousePos.x = ( ( mX + 1.0f ) / 2.0f ) * SCREEN_WIDTH;
    imguiIO.MousePos.y = ( 1.0f - ( ( mY + 1.0f ) / 2.0f ) ) * SCREEN_HEIGHT;
    imguiIO.MouseDown[0] = i->mouseButtons[0];

    char* keysInputted = i->keysPressedSinceLastUpdate;
    /*  24 here is a magic number corresponding to InputState struct's 
        keysPressedSinceLastUpdate array length                        */
    for( int charIndex = 0; charIndex < 24; ++charIndex ) {
        if( keysInputted[ charIndex ] != 0 ) {
            imguiIO.AddInputCharacter( keysInputted[ charIndex ] );
        }
    }

    imguiIO.KeysDown[ InputState::BACKSPACE ] = i->spcKeys[ InputState::BACKSPACE ];
    imguiIO.KeysDown[ InputState::TAB ] = i->spcKeys[ InputState::TAB ];

    ImguiLimestoneDriver* imData = (ImguiLimestoneDriver*)imguiIO.UserData;
}

static void* InitImGui_LimeStone( 
    Stack* allocater, 
    RenderDriver* rDriver
) {
    ImguiLimestoneDriver* imguiDriver = (ImguiLimestoneDriver*)
        StackAlloc( allocater, sizeof( ImguiLimestoneDriver ) );
    imguiDriver->renderDriver = rDriver;
    imguiDriver->mem = allocater;

    size_t imguiStateSize = ImGui::GetInternalStateSize();
    void* imguiState = StackAllocA( allocater, imguiStateSize );
    ImGui::SetInternalState( imguiState, true );

    rDriver->CreateShaderProgram(
        rDriver,
        ImGuiVertShaderSrc, 
        ImGuiFragShaderSrc, 
        &imguiDriver->ImGuiUIShader 
    );
    imguiDriver->imguiVertexDataPtr = rDriver->AllocGpuBuffForDynamicStream( rDriver );

    ImGuiIO& imguiIO = ImGui::GetIO();
    imguiIO.DisplaySize.x = SCREEN_WIDTH;
    imguiIO.DisplaySize.y = SCREEN_HEIGHT;
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
        rDriver, 
        &imguiTexData, 
        &imguiDriver->imguiFontTexturePtr
    );
    imguiIO.Fonts->TexID = (void*)imguiDriver->imguiFontTexturePtr;
    imguiIO.Fonts->ClearInputData();
    imguiIO.Fonts->ClearTexData();

    return imguiState;
}

#if 0
struct InAppConsole
{
    ImVector<char*>       Items;
    bool                  ScrollToBottom;
    ImVector<char*>       History;
    // -1: new line, 0..History.size()-1 browsing history.
    int                   HistoryPos;    
    ImVector<const char*> Commands;

    InAppConsole()
    {
        Items = ImVector<char*>();
        ClearLog();
        HistoryPos = -1;
        Commands.push_back("HISTORY");
        Commands.push_back("CLEAR");
    }
    ~InAppConsole()
    {
        ClearLog();
        for (size_t i = 0; i < Items.size(); i++) 
            free(History[i]); 
    }

    void ClearLog()
    {
        for (size_t i = 0; i < Items.size(); i++)  {
            free(Items[i]); 
        }
        Items.clear();
        ScrollToBottom = true;
    }

    void AddLog(const char* fmt, ...)
    {
        char buf[1024];
        va_list args;
        va_start(args, fmt);
        ImFormatStringV(buf, IM_ARRAYSIZE(buf), fmt, args);
        va_end(args);
        Items.push_back(strdup(buf));
        ScrollToBottom = true;
    }

    void Run(const char* title, bool* opened)
    {
        ImGui::SetNextWindowSize(ImVec2(520,600), ImGuiSetCond_FirstUseEver);
        if (!ImGui::Begin(title, opened))
        {
            ImGui::End();
            return;
        }

        ImGui::BeginChild( "Command Buttons", ImVec2(0,ImGui::GetTextLineHeightWithSpacing()));
        ImGui::SameLine(); 
        if (ImGui::SmallButton("Clear")) {
            ClearLog();
        }
        ImGui::EndChild();

        ImGui::Separator();

        ImGui::BeginChild("ScrollingRegion", ImVec2(0,-ImGui::GetTextLineHeightWithSpacing()*2));
        if (ImGui::BeginPopupContextWindow())
        {
            if (ImGui::Selectable("Clear")) 
                ClearLog();
            ImGui::EndPopup();
        }
        for (size_t i = 0; i < Items.size(); i++)
        {
            const char* item = Items[i];
            ImVec4 col(1,1,1,1);
            if (strstr(item, "[error]")) col = ImVec4(1.0f,0.4f,0.4f,1.0f);
            else if (strncmp(item, "# ", 2) == 0) col = ImVec4(1.0f,0.8f,0.6f,1.0f);
            ImGui::PushStyleColor(ImGuiCol_Text, col);
            ImGui::TextUnformatted(item);
            ImGui::PopStyleColor();
        }
        ImGui::EndChild();

        ImGui::End();
    }
};
#endif