// ============================================================
//  ShineAssetTest — unit tests for the new asset system.
//
//  Tests covered:
//    1. AssetDependencyGraph cycle detection
//    2. EditorAssetRegistry scan / relocate / delete
//    3. AssetMetadata round-trip serialization
//    4. RuntimeAssetRegistry thread-safe load / find / evict
// ============================================================

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "../../src/string/shine_string.h"
#include "../common/test_benchmark_framework.h"
#include "fmt/base.h"
#include "fmt/format.h"

#include "editor/ShineAsset/AssetDependencyGraph.h"
#include "editor/ShineAsset/AssetMetadata.h"
#include "editor/ShineAsset/RuntimeAssetRegistry.h"
#include "editor/ShineAsset/EditorAssetRegistry.h"
#include "editor/ShineAsset/AssetUuidHelper.h"
#include "editor/ShineAsset/AssetTypes.h"

static shine::test::TestContext g_ctx;
#define CHECK(name, cond) SHINE_TEST_CHECK(g_ctx, name, cond)

using namespace shine;
using namespace shine::editor::asset;
using namespace shine::asset;

// ============================================================
//  1. AssetDependencyGraph Tests
// ============================================================

void test_dependency_graph_basic()
{
    AssetDependencyGraph graph;

    std::vector<std::string> deps_a = { "uuid-b", "uuid-c" };
    graph.SetDependencies("uuid-a", deps_a);

    CHECK("forward deps count", graph.GetDependencies("uuid-a").size() == 2);
    CHECK("reverse dep b has a", graph.GetDependents("uuid-b").contains(SString("uuid-a")));
    CHECK("reverse dep c has a", graph.GetDependents("uuid-c").contains(SString("uuid-a")));
    CHECK("no dependents for a", graph.GetDependents("uuid-a").empty());
    CHECK("has dependents b", graph.HasDependents("uuid-b"));
}

void test_dependency_graph_cycle_detection()
{
    AssetDependencyGraph graph;

    // A → B → C
    std::vector<std::string> deps_a = { "uuid-b" };
    std::vector<std::string> deps_b = { "uuid-c" };
    graph.SetDependencies("uuid-a", deps_a);
    graph.SetDependencies("uuid-b", deps_b);

    // Adding C → A would create a cycle
    CHECK("cycle detection C→A", graph.WouldCreateCycle("uuid-c", "uuid-a") == true);
    CHECK("no cycle C→D", graph.WouldCreateCycle("uuid-c", "uuid-d") == false);
    CHECK("self-cycle A→A", graph.WouldCreateCycle("uuid-a", "uuid-a") == true);
}

void test_dependency_graph_remove()
{
    AssetDependencyGraph graph;

    std::vector<std::string> deps = { "uuid-b" };
    graph.SetDependencies("uuid-a", deps);
    CHECK("b has dependent before remove", graph.HasDependents("uuid-b"));

    graph.RemoveAsset("uuid-a");
    CHECK("b has no dependent after remove", !graph.HasDependents("uuid-b"));
    CHECK("a forward deps empty", graph.GetDependencies("uuid-a").empty());
}

void test_dependency_graph_update()
{
    AssetDependencyGraph graph;

    std::vector<std::string> deps1 = { "uuid-b", "uuid-c" };
    graph.SetDependencies("uuid-a", deps1);
    CHECK("initial deps count", graph.GetDependencies("uuid-a").size() == 2);

    // Update: remove c, add d
    std::vector<std::string> deps2 = { "uuid-b", "uuid-d" };
    graph.SetDependencies("uuid-a", deps2);
    CHECK("updated deps count", graph.GetDependencies("uuid-a").size() == 2);
    CHECK("c no longer has dependent", !graph.HasDependents("uuid-c"));
    CHECK("d now has dependent", graph.HasDependents("uuid-d"));
}

// ============================================================
//  2. AssetMetadata Round-Trip Tests
// ============================================================

void test_metadata_roundtrip()
{
    AssetMetadata meta;
    meta.formatVersion = "2.0";
    meta.asset.uuid = "test-uuid-1234";
    meta.asset.type = std::string(AssetTypeId::Model);
    meta.asset.sourceFile = "models/character.glb";
    meta.asset.imported = true;
    meta.asset.lastImportTime = "2026-03-13T12:00:00Z";

    SubAssetEntry sub;
    sub.uuid = "sub-mesh-uuid";
    sub.type = std::string(SubAssetTypeId::Mesh);
    sub.name = "MainMesh";
    sub.properties = glz::raw_json{R"({"vertexCount":1000})"};
    meta.asset.subAssets.push_back(std::move(sub));

    meta.asset.dependencies = { "dep-uuid-1", "dep-uuid-2" };

    // Serialize
    auto writeResult = WriteAssetMetadata(meta);
    CHECK("write succeeded", writeResult.has_value());

    // Deserialize
    auto readResult = ReadAssetMetadata(writeResult.value());
    CHECK("read succeeded", readResult.has_value());

    const auto& parsed = readResult.value();
    CHECK("format version", parsed.formatVersion == "2.0");
    CHECK("uuid match", parsed.asset.uuid == "test-uuid-1234");
    CHECK("type match", parsed.asset.type == std::string(AssetTypeId::Model));
    CHECK("sourceFile match", parsed.asset.sourceFile == "models/character.glb");
    CHECK("imported flag", parsed.asset.imported == true);
    CHECK("sub-assets count", parsed.asset.subAssets.size() == 1);
    CHECK("sub-asset name", parsed.asset.subAssets[0].name == "MainMesh");
    CHECK("dependencies count", parsed.asset.dependencies.size() == 2);
}

void test_metadata_file_roundtrip()
{
    // Create a temp directory for the test
    auto tempDir = std::filesystem::temp_directory_path() / "shine_asset_test";
    std::filesystem::create_directories(tempDir);
    auto tempFile = tempDir / "test.sasset";

    AssetMetadata meta;
    meta.asset.uuid = "file-test-uuid";
    meta.asset.type = std::string(AssetTypeId::Texture);

    auto writeResult = WriteAssetMetadataFile(meta, tempFile.string());
    CHECK("file write succeeded", writeResult.has_value());
    CHECK("file exists", std::filesystem::exists(tempFile));

    auto readResult = ReadAssetMetadataFile(tempFile.string());
    CHECK("file read succeeded", readResult.has_value());
    CHECK("file uuid match", readResult->asset.uuid == "file-test-uuid");

    // Cleanup
    std::filesystem::remove_all(tempDir);
}

// ============================================================
//  3. RuntimeAssetRegistry Tests
// ============================================================

class TestAsset : public AssetBase
{
public:
    explicit TestAsset(STextView uuid)
        : AssetBase(uuid, "test_type")
    {
    }
    int testData = 42;
};

void test_runtime_registry_basic()
{
    RuntimeAssetRegistry registry;

    // Register a factory
    registry.RegisterFactory("test_type", [](STextView uuid) -> std::shared_ptr<AssetBase> {
        return std::make_shared<TestAsset>(uuid);
    });

    // Register an asset
    auto asset = std::make_shared<TestAsset>("test-uuid-1");
    asset->SetState(EAssetState::Loaded);
    registry.Register(asset);

    CHECK("contains registered", registry.Contains("test-uuid-1"));
    CHECK("find returns asset", registry.Find("test-uuid-1") != nullptr);
    CHECK("find typed works", registry.FindAs<TestAsset>("test-uuid-1") != nullptr);
    CHECK("find typed data", registry.FindAs<TestAsset>("test-uuid-1")->testData == 42);
    CHECK("count is 1", registry.AssetCount() == 1);

    // Unregister
    registry.Unregister("test-uuid-1");
    CHECK("not found after unregister", !registry.Contains("test-uuid-1"));
    CHECK("count is 0", registry.AssetCount() == 0);
}

void test_runtime_registry_request_load()
{
    RuntimeAssetRegistry registry;

    registry.RegisterFactory("test_type", [](STextView uuid) -> std::shared_ptr<AssetBase> {
        return std::make_shared<TestAsset>(uuid);
    });

    // Request load for new asset
    auto [result, asset] = registry.RequestLoad("load-test-uuid", "test_type");
    CHECK("request queued", result == ELoadResult::Queued);
    CHECK("placeholder not null", asset != nullptr);
    CHECK("placeholder loading", asset->IsLoading());

    // Second request for same UUID
    auto [result2, asset2] = registry.RequestLoad("load-test-uuid", "test_type");
    CHECK("already loading", result2 == ELoadResult::AlreadyLoading);

    // Mark loaded and try again
    asset->SetState(EAssetState::Loaded);
    auto [result3, asset3] = registry.RequestLoad("load-test-uuid", "test_type");
    CHECK("already loaded", result3 == ELoadResult::AlreadyLoaded);

    // No factory
    auto [result4, asset4] = registry.RequestLoad("new-uuid", "unknown_type");
    CHECK("no factory", result4 == ELoadResult::NoFactory);

    // Invalid UUID
    auto [result5, asset5] = registry.RequestLoad("", "test_type");
    CHECK("invalid uuid", result5 == ELoadResult::InvalidUUID);
}

void test_runtime_registry_thread_safety()
{
    RuntimeAssetRegistry registry;

    registry.RegisterFactory("test_type", [](STextView uuid) -> std::shared_ptr<AssetBase> {
        return std::make_shared<TestAsset>(uuid);
    });

    constexpr int numThreads = 4;
    constexpr int assetsPerThread = 50;
    std::vector<std::thread> threads;

    // Concurrently register assets
    for (int t = 0; t < numThreads; ++t)
    {
        threads.emplace_back([&registry, t]() {
            for (int i = 0; i < assetsPerThread; ++i)
            {
                SString uuid = SString(fmt::format("thread-{}-asset-{}", t, i));
                auto asset = std::make_shared<TestAsset>(uuid);
                asset->SetState(EAssetState::Loaded);
                registry.Register(std::move(asset));
            }
        });
    }

    for (auto& th : threads)
        th.join();

    CHECK("all assets registered", registry.AssetCount() == numThreads * assetsPerThread);

    // Concurrently find assets
    threads.clear();
    std::atomic<int> foundCount{0};
    for (int t = 0; t < numThreads; ++t)
    {
        threads.emplace_back([&registry, &foundCount, t]() {
            for (int i = 0; i < assetsPerThread; ++i)
            {
                SString uuid = SString(fmt::format("thread-{}-asset-{}", t, i));
                if (registry.Find(uuid))
                    foundCount.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    for (auto& th : threads)
        th.join();

    CHECK("all assets found concurrently", foundCount.load() == numThreads * assetsPerThread);

    registry.Clear();
    CHECK("clear empties registry", registry.AssetCount() == 0);
}

// ============================================================
//  4. EditorAssetRegistry Tests
// ============================================================

void test_editor_registry_scan()
{
    // Create temp content directory with .sasset files
    auto tempDir = std::filesystem::temp_directory_path() / "shine_editor_test";
    std::filesystem::create_directories(tempDir);

    // Write a test .sasset file
    AssetMetadata meta1;
    meta1.asset.uuid = "scan-uuid-1";
    meta1.asset.type = std::string(AssetTypeId::Model);
    meta1.asset.sourceFile = "models/test.glb";
    WriteAssetMetadataFile(meta1, (tempDir / "test1.sasset").string());

    AssetMetadata meta2;
    meta2.asset.uuid = "scan-uuid-2";
    meta2.asset.type = std::string(AssetTypeId::Texture);
    meta2.asset.dependencies = { "scan-uuid-1" };
    WriteAssetMetadataFile(meta2, (tempDir / "test2.sasset").string());

    // Scan
    EditorAssetRegistry registry;
    auto count = registry.Scan(tempDir);
    CHECK("scan found 2 assets", count == 2);
    CHECK("entry count", registry.EntryCount() == 2);
    CHECK("find by uuid 1", registry.Find("scan-uuid-1") != nullptr);
    CHECK("find by uuid 2", registry.Find("scan-uuid-2") != nullptr);
    CHECK("find by path", registry.FindByPath(tempDir / "test1.sasset") != nullptr);
    CHECK("is known", registry.IsKnown("scan-uuid-1"));

    // Dependency graph was built
    CHECK("dep graph: uuid-2 depends on uuid-1", registry.GetDependents("scan-uuid-1").contains(SString("scan-uuid-2")));

    // Cleanup
    std::filesystem::remove_all(tempDir);
}

void test_editor_registry_relocate()
{
    auto tempDir = std::filesystem::temp_directory_path() / "shine_relocate_test";
    std::filesystem::create_directories(tempDir);
    std::filesystem::create_directories(tempDir / "subdir");

    AssetMetadata meta;
    meta.asset.uuid = "relocate-uuid";
    meta.asset.type = std::string(AssetTypeId::Model);
    auto oldPath = tempDir / "old.sasset";
    auto newPath = tempDir / "subdir" / "new.sasset";
    WriteAssetMetadataFile(meta, oldPath.string());

    EditorAssetRegistry registry;
    registry.Scan(tempDir);

    CHECK("found before move", registry.FindByPath(oldPath) != nullptr);

    // Simulate file move
    std::filesystem::rename(oldPath, newPath);
    registry.OnFileMoved(oldPath, newPath);

    CHECK("not found at old path", registry.FindByPath(oldPath) == nullptr);
    CHECK("found at new path", registry.FindByPath(newPath) != nullptr);
    CHECK("uuid still valid", registry.IsKnown("relocate-uuid"));

    std::filesystem::remove_all(tempDir);
}

void test_editor_registry_delete()
{
    auto tempDir = std::filesystem::temp_directory_path() / "shine_delete_test";
    std::filesystem::create_directories(tempDir);

    AssetMetadata meta1;
    meta1.asset.uuid = "delete-uuid-1";
    meta1.asset.type = std::string(AssetTypeId::Model);
    WriteAssetMetadataFile(meta1, (tempDir / "dep.sasset").string());

    AssetMetadata meta2;
    meta2.asset.uuid = "delete-uuid-2";
    meta2.asset.type = std::string(AssetTypeId::Material);
    meta2.asset.dependencies = { "delete-uuid-1" };
    WriteAssetMetadataFile(meta2, (tempDir / "mat.sasset").string());

    EditorAssetRegistry registry;
    registry.Scan(tempDir);

    // Try safe delete of asset with dependents — should fail
    auto result = registry.TryDelete("delete-uuid-1", EDeletePolicy::SafeOnly);
    CHECK("safe delete refused", !result.succeeded);
    CHECK("affected dependents count", result.affectedDependents.size() == 1);

    // Force delete
    auto result2 = registry.TryDelete("delete-uuid-1", EDeletePolicy::Force);
    CHECK("force delete succeeded", result2.succeeded);
    CHECK("entry removed", registry.Find("delete-uuid-1") == nullptr);

    std::filesystem::remove_all(tempDir);
}

// ============================================================
//  main
// ============================================================

int main()
{
    fmt::println("╔══════════════════════════════════════════════════════════════════╗");
    fmt::println("║           ShineEngine — ShineAsset System Tests                ║");
    fmt::println("╚══════════════════════════════════════════════════════════════════╝\n");

    fmt::println("--- AssetDependencyGraph ---");
    test_dependency_graph_basic();
    test_dependency_graph_cycle_detection();
    test_dependency_graph_remove();
    test_dependency_graph_update();

    fmt::println("\n--- AssetMetadata Serialization ---");
    test_metadata_roundtrip();
    test_metadata_file_roundtrip();

    fmt::println("\n--- RuntimeAssetRegistry ---");
    test_runtime_registry_basic();
    test_runtime_registry_request_load();
    test_runtime_registry_thread_safety();

    fmt::println("\n--- EditorAssetRegistry ---");
    test_editor_registry_scan();
    test_editor_registry_relocate();
    test_editor_registry_delete();

    fmt::println("");
    g_ctx.print_summary("ShineAsset System Tests");
    return g_ctx.all_passed() ? 0 : 1;
}
