#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

#ifndef SHINE_IMPORT_STD
#include <mutex>
#include <vector>
#include <queue>
#endif

export module shine.manager.file_watch_manager;

#ifdef SHINE_IMPORT_STD
#include "std.h"
#endif


#include "shine_define/h"
#include "util/singleton/h"
#include "util/string_util.h"

namespace shine::manager
{
    enum class EWatchType
    {
        Add,        // file added
        Remove,     // file removed
        Modify,     // file modified
        RenameOld, // file renamed old
        RenameNew,  // file renamed new
    };


    struct FWatchEvent
    {
        EWatchType type;
        std::wstring path;
    };

    struct FWatchContext
    {
        HANDLE handle;          // file handle
        OVERLAPPED overlapped;  // overlapped IO  structure

        std::wstring path;
        std::mutex   _mutex;                       // mutex for thread safety
        std::atomic<bool> isMonitoring;



        std::vector<unsigned char> buffer;        // event buffer
        std::queue<FWatchEvent> pendingEvents;  //Event List


        FWatchContext(){
            overlapped = {};
            handle = nullptr;
            path = {};
            buffer.clear();
            isMonitoring = false;
        }

        bool MakeContent(std::string_view _path){

            path = util::StringUtil::UTF8ToWstring(_path);

            handle = CreateFileW(
                path.c_str(),
                FILE_LIST_DIRECTORY,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                nullptr,
                OPEN_EXISTING,
                FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,  // 鏀寔鐩綍鍜屽紓姝?
                nullptr
            );

            if(handle == INVALID_HANDLE_VALUE)
            {
                return  false;
            }

            ZeroMemory(&overlapped, sizeof(overlapped));

        }


        bool StartWatch(){
            if(isMonitoring){ return false; }

            unsigned long _size = 0;
            const bool result = ReadDirectoryChangesW(
                handle,
                buffer.data(),
                static_cast<DWORD>(buffer.size()),
                false,
                FILE_NOTIFY_CHANGE_FILE_NAME  |   // 鏂囦欢鍚嶅彉鍖栵紙鏂板/鍒犻櫎/閲嶅懡鍚嶏級
                FILE_NOTIFY_CHANGE_DIR_NAME   |    // 鐩綍鍚嶅彉鍖?
                FILE_NOTIFY_CHANGE_LAST_WRITE |  // 鏈€鍚庡啓鍏ユ椂闂村彉鍖栵紙淇敼锛?
                FILE_NOTIFY_CHANGE_CREATION   | // 鏂囦欢鍒涘缓鏃堕棿鍙樺寲
                FILE_NOTIFY_CHANGE_SIZE  , // 鏂囦欢澶у皬鍙樺寲
                &_size,
                &overlapped,
                nullptr
            );

            if(!result){
                return true;
            }

            isMonitoring = true;
            return false;
        }

        void StopWatch()
        {
            if(!isMonitoring){ return; }

            isMonitoring = false;

            if(handle != INVALID_HANDLE_VALUE){
                CancelIoEx(handle, &overlapped);  // 鍙栨秷鏈畬鎴愮殑 I/O
                CloseHandle(handle);
            }

        }

        void ParseEvent(DWORD bytes)
        {
            if(bytes == 0){ return; }

            unsigned char* data = buffer.data();

            while(bytes > 0)
            {
                auto* notify = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(data);
                if(!notify) break;

                // 璁＄畻褰撳墠浜嬩欢鍗犵敤鐨勫瓧鑺傛暟
                const DWORD entrySize = notify->NextEntryOffset + notify->FileNameLength;
                if (entrySize > bytes) break;

                const std::wstring fullPath = path.append(std::wstring(notify->FileName, notify->FileNameLength / sizeof(WCHAR)));


                // 鏄犲皠浜嬩欢绫诲瀷
                EWatchType type;
                switch (notify->Action) {
                    case FILE_ACTION_ADDED:      type = EWatchType::Add; break;
                    case FILE_ACTION_REMOVED:    type = EWatchType::Remove; break;
                    case FILE_ACTION_MODIFIED:   type = EWatchType::Modify; break;
                    case FILE_ACTION_RENAMED_OLD_NAME:  type = EWatchType::RenameOld; break;
                    case FILE_ACTION_RENAMED_NEW_NAME:  type = EWatchType::RenameNew; break;
                    default:  /* 蹇界暐鏈煡浜嬩欢 */ break;
                }

                // 鍔犻攣淇濇姢浜嬩欢闃熷垪
                std::lock_guard<std::mutex> lock(_mutex);
                pendingEvents.emplace(FWatchEvent{type, fullPath});

                // 绉诲姩鍒颁笅涓€涓簨浠?
                data += notify->NextEntryOffset;
                bytes -= entrySize;
            }
        }
    };

    class FileWatchManager : public shine::util::Singleton<FileWatchManager>
    {

    public:


    };

}

