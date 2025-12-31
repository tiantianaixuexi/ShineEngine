#include "GL/glew.h"

#include "third/stb/stb_image.h"


#include "imgui.h"


namespace shine::util
{

    bool LoadTextureFromMemory(const void *data, size_t data_size, GLuint *out_texture, int *out_width, int *out_height);
    bool LoadTextureFromFile(const char *file_name, GLuint *out_texture, int *out_width, int *out_height);

}

 namespace shine::util
 {

    bool LoadTextureFromMemory(const void *data, size_t data_size, GLuint *out_texture, int *out_width, int *out_height)
    {
        // Load from file
        int image_width = 0;
        int image_height = 0;
        unsigned char *image_data = stbi_load_from_memory(static_cast<const unsigned char*>(data), static_cast<int>(data_size), &image_width, &image_height, nullptr, 4);
        if (image_data == nullptr)
            return false;

        // Create a OpenGL texture identifier
        GLuint image_texture;
        glGenTextures(1, &image_texture);
        glBindTexture(GL_TEXTURE_2D, image_texture);

        // Setup filtering parameters for display
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Upload pixels into texture
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
        stbi_image_free(image_data);

        *out_texture = image_texture;
        *out_width = image_width;
        *out_height = image_height;

        return true;
    }

    bool LoadTextureFromFile(const char *file_name, GLuint *out_texture, int *out_width, int *out_height)
    {
        FILE *f = nullptr;
        if(fopen_s(&f,file_name, "rb") !=0 ) return false;
        fseek(f, 0, 2);
        const size_t file_size = static_cast<size_t>(ftell(f));
        if (file_size == -1)
            return false;
        fseek(f, 0, 0);
        void *file_data = ImGui::MemAlloc(file_size);
        fread(file_data, 1, file_size, f);
        fclose(f);
        const bool ret = LoadTextureFromMemory(file_data, file_size, out_texture, out_width, out_height);
        ImGui::MemFree(file_data);
        return ret;
    }

 

}
