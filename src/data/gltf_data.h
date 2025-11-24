#ifndef SHINE_IMPORT_STD
#include <string>
#endif

export module shine.model.gltf;

#ifdef SHINE_IMPORT_STD
#include "std.h"
#endif

namespace shine::data{


    struct Accessor{

        int bufferView{-1};
        std::string name;
        size_t byteOffset{0};
        bool normalized{false};
        int componentType{-1};
        size_t count{0};
        int type{-1};


        std::string extras_json_string;
        std::string extensions_json_string;
    };

}

