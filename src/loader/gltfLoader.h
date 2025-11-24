#pragma once

#include "loader.h"


namespace shine::loader
{

    class  gltfLoader : public IAssetLoader
    {

        public:
           
            gltfLoader() {
                    addSupportedExtension("gltf");
                    addSupportedExtension("glb");
            }
        
            virtual ~gltfLoader() = default; 
            virtual  bool loadFromMemory(const void* data, size_t size) ;
            virtual  bool loadFromFile(const char* filePath) ;
            void unload() ;

          

    };

 
}

