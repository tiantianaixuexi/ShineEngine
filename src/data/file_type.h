export  module shine.data.file_type;


namespace shine::data
{

    enum class EFileType
    {
        Image,
        Video,
        Audio,
        json,
    };


    enum class EImageSuffix
    {
        JPG,
        PNG,
        GIF,
        BMP,
        TIFF,
        WEBP,
        ICO,
        SVG
    };

}
