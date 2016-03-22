bool InitRenderer( uint16 screen_w, uint16 screen_h ) {
	printf( "Vendor: %s\n", glGetString( GL_VENDOR ) );
    printf( "Renderer: %s\n", glGetString( GL_RENDERER ) );
    printf( "GL Version: %s\n", glGetString( GL_VERSION ) );
    printf( "GLSL Version: %s\n", glGetString( GL_SHADING_LANGUAGE_VERSION ) );

    GLint k;
    glGetIntegerv( GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &k );
    printf("Max texture units: %d\n", k);

    //Initialize clear color
    glClearColor( 0.1f, 0.1f, 0.1f, 1.0f );

    //glEnable( GL_DEPTH_TEST );
    glEnable( GL_CULL_FACE );
    glCullFace( GL_BACK );

    glViewport( 0.0f, 0.0f, screen_w, screen_h );

    float screenAspectRatio = screen_w / screen_h;
    //InitOrthoCameraMatrix( &baseOrthoMat, 20.0f * screenAspectRatio, 20.0f, -20.0f, 20.0f );
    //InitOrthoCameraMatrix( &baseOrthoMat, screen_w, screen_h, -1.0f, 1.0f );
    //cameraPosition = {0.0f, 0.0f, 0.0f};

    //Initialize framebuffer
    //CreateFrameBuffer( &myFramebuffer );
    //CreateShader( &framebufferShader, "Data/Shaders/Framebuffer.vert", "Data/Shaders/Framebuffer.frag" );

    //Check for error
    GLenum error = glGetError();
    if( error != GL_NO_ERROR ) {
        printf( "Error initializing OpenGL! %s\n", gluErrorString( error ) );
        return false;
    } else {
        printf( "Initialized OpenGL\n" );
    }

    return true;
}