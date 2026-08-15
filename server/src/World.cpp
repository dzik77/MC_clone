#include "World.hpp"
#include <cstring>

World::World() {

    // Initialize noise generators for terrain generation 
    continentalness = FastNoise::New<FastNoise::Simplex>();
    continentalness->SetScale(1000.f);
    
    erosion = FastNoise::New<FastNoise::Simplex>();
    erosion->SetScale(250.f);
    erosionFractal = FastNoise::New<FastNoise::FractalFBm>();
    erosionFractal->SetSource(erosion);
    erosionFractal->SetOctaveCount(5);
    erosionFractal->SetLacunarity(2.2f);
    erosionFractal->SetGain(0.3f);

    peaksAndValleys = FastNoise::New<FastNoise::Simplex>();
    peaksAndValleys->SetScale(500.f);
    peaksAndValleysFractal = FastNoise::New<FastNoise::FractalFBm>();
    peaksAndValleysFractal->SetSource(peaksAndValleys);
    peaksAndValleysFractal->SetOctaveCount(3);

    // ---------------------------------------------------------
    // 1. Continentalness (Base Heights)
    // ---------------------------------------------------------
    // Oceans floor at Y=20, highest inland plateaus base at Y=90.
    continentalnessSpline.addPoint({-1.0f, 30.0f});  // Deep Ocean Floor
    continentalnessSpline.addPoint({-0.5f, 62.0f});   // Coastline / Sea level (usually 62/63 in MC)
    continentalnessSpline.addPoint({0.8f, 85.0f});   // Inland Plateau
    continentalnessSpline.addPoint({1.0f, 90.0f});   // High Mainland

    // ---------------------------------------------------------
    // 2. Erosion (The Multiplier)
    // ---------------------------------------------------------
    // Remember: This does NOT output block height! It outputs a scalar (multiplier)
    // that determines how heavily the Peaks and Valleys are allowed to affect the terrain.
    erosionSpline.addPoint({-1.0f, 1.6f});   // Low erosion: Allow massive PV scaling (Mountains)
    erosionSpline.addPoint({0.0f, 0.5f});    // Mid erosion: Standard rolling hills
    erosionSpline.addPoint({1.0f, 0.0f});    // High erosion: Completely nullify PV (Plains/Swamps)

    // ---------------------------------------------------------
    // 3. Peaks and Valleys (The Amplitude)
    // ---------------------------------------------------------
    // The maximum localized block variation allowed to be added/subtracted 
    // from the Continentalness base height.
    peaksAndValleysSpline.addPoint({-1.0f, -12.0f}); // Low PV: Deep river trenches (max -12 * 1.6 = -19 blocks)
    peaksAndValleysSpline.addPoint({0.0f, 0.0f});    // Mid PV: No localized effect
    peaksAndValleysSpline.addPoint({0.6f, 40.0f});   // High-ish PV: Foothills
    peaksAndValleysSpline.addPoint({1.0f, 100.0f});  // Extreme PV: Sharp spires (max 100 * 1.6 = +160 blocks)

}

/* deprecated
void World::getTerrain(glm::ivec2 chunkColumnPosition, ChunkPacket* chunkColumnPacket) {
    
    bool chunkColumnExists;
    {
        std::lock_guard<std::mutex> lock(chunkColumnsMutex);
        chunkColumnExists = chunkColumns.find(chunkColumnPosition) != chunkColumns.end();

    }
    if (!chunkColumnExists) {
        generateTerrain(chunkColumnPosition);
    }

    for (int y = 0; y < WorldConfig::CHUNK_COLUMN_HEIGHT; y++){
        chunkColumnPacket[y].x = chunkColumnPosition.x;
        chunkColumnPacket[y].z = chunkColumnPosition.y;
        chunkColumnPacket[y].y = y;
        std::memset(chunkColumnPacket[y].chunkBlocks, 0, sizeof(chunkColumnPacket[y].chunkBlocks));
    }
    
    {
        std::lock_guard<std::mutex> lock(chunkColumnsMutex);
        auto& chunkColumn = chunkColumns[chunkColumnPosition];
        chunkColumn->playersLoadedChunkColumn++; // Increment the count of players that have loaded this chunk column
        for (int x = 0; x < WorldConfig::CHUNK_SIZE; x++){
            for (int y = 0; y < WorldConfig::CHUNK_SIZE * WorldConfig::CHUNK_COLUMN_HEIGHT; y++){
                int chunkY = y / WorldConfig::CHUNK_SIZE;
                int localY = y % WorldConfig::CHUNK_SIZE;
                for (int z = 0; z < WorldConfig::CHUNK_SIZE; z++){
                    chunkColumnPacket[chunkY].chunkBlocks[x * WorldConfig::CHUNK_SIZE * WorldConfig::CHUNK_SIZE + localY * WorldConfig::CHUNK_SIZE + z] = chunkColumn->blocks[x][y][z];
                }
            }
        }
    }

}
*/

void World::loadChunkColumn(glm::ivec2 chunkColumnPosition, ENetPeer* peer) {

    ChunkToGenerate chunkToGenerate;
    chunkToGenerate.x = chunkColumnPosition.x;
    chunkToGenerate.z = chunkColumnPosition.y;
    chunkToGenerate.send = false;
    chunkToGenerate.peer = peer;
    {
        std::lock_guard<std::mutex> lock(chunksToGenerateMutex);
        chunksToGenerate.push(chunkToGenerate);
        chunksToGenerateCV.notify_one();
    }

}

void World::sentAndLoadChunkColumn(glm::ivec2 chunkColumnPosition, ENetPeer* peer) {

    ChunkToGenerate chunkToGenerate;
    chunkToGenerate.x = chunkColumnPosition.x;
    chunkToGenerate.z = chunkColumnPosition.y;
    chunkToGenerate.send = true;
    chunkToGenerate.peer = peer;
    {
        std::lock_guard<std::mutex> lock(chunksToGenerateMutex);
        chunksToGenerate.push(chunkToGenerate);
        chunksToGenerateCV.notify_one();
    }

}

ChunkPacket* World::chunkColumnToPacket(ChunkColumn& chunkColumn){
    ChunkPacket* chunkColumnPacket = new ChunkPacket[WorldConfig::CHUNK_COLUMN_HEIGHT];

    for (int y = 0; y < WorldConfig::CHUNK_COLUMN_HEIGHT; y++){
        chunkColumnPacket[y].x = chunkColumn.position.x;
        chunkColumnPacket[y].z = chunkColumn.position.y;
        chunkColumnPacket[y].y = y;
        std::memset(chunkColumnPacket[y].chunkBlocks, 0, sizeof(chunkColumnPacket[y].chunkBlocks));
    }
    
    for (int x = 0; x < WorldConfig::CHUNK_SIZE; x++){
        for (int y = 0; y < WorldConfig::CHUNK_SIZE * WorldConfig::CHUNK_COLUMN_HEIGHT; y++){
            int chunkY = y / WorldConfig::CHUNK_SIZE;
            int localY = y % WorldConfig::CHUNK_SIZE;
            for (int z = 0; z < WorldConfig::CHUNK_SIZE; z++){
                chunkColumnPacket[chunkY].chunkBlocks[x * WorldConfig::CHUNK_SIZE * WorldConfig::CHUNK_SIZE + localY * WorldConfig::CHUNK_SIZE + z] = chunkColumn.blocks[x][y][z];
            }
        }
    }

    return chunkColumnPacket;
}

void World::generateTerrain(glm::ivec2 chunkColumnPosition, ENetPeer* peer) {

    // Generate noise values for the chunk column (SIMD optimized)
    std::vector<float> continentalnessBuffer(WorldConfig::CHUNK_SIZE * WorldConfig::CHUNK_SIZE), erosionBuffer(WorldConfig::CHUNK_SIZE * WorldConfig::CHUNK_SIZE), peaksAndValleysBuffer(WorldConfig::CHUNK_SIZE * WorldConfig::CHUNK_SIZE), chunkBuffer(WorldConfig::CHUNK_SIZE * WorldConfig::CHUNK_SIZE);
    continentalness->GenUniformGrid2D(continentalnessBuffer.data(), chunkColumnPosition.x * WorldConfig::CHUNK_SIZE, chunkColumnPosition.y * WorldConfig::CHUNK_SIZE, WorldConfig::CHUNK_SIZE, WorldConfig::CHUNK_SIZE, 1.f, 1.f, seed);
    erosionFractal->GenUniformGrid2D(erosionBuffer.data(), chunkColumnPosition.x * WorldConfig::CHUNK_SIZE, chunkColumnPosition.y * WorldConfig::CHUNK_SIZE, WorldConfig::CHUNK_SIZE, WorldConfig::CHUNK_SIZE, 1.f, 1.f, seed);
    peaksAndValleysFractal->GenUniformGrid2D(peaksAndValleysBuffer.data(), chunkColumnPosition.x * WorldConfig::CHUNK_SIZE, chunkColumnPosition.y * WorldConfig::CHUNK_SIZE, WorldConfig::CHUNK_SIZE, WorldConfig::CHUNK_SIZE, 1.f, 1.f, seed);

    ChunkToSend chunkToSend;
    auto& chunkColumn = chunkToSend.chunkPacket;
    chunkToSend.upload = true;
    chunkToSend.peer = peer;

    // Combine the noise values to determine the final height of each column in the chunk
    for (int x = 0; x < WorldConfig::CHUNK_SIZE; x++){
        for (int z = 0; z < WorldConfig::CHUNK_SIZE; z++){
            float continentalnessValue = continentalnessSpline.evaluateHeight(continentalnessBuffer[z * WorldConfig::CHUNK_SIZE + x]);
            float erosionValue = erosionSpline.evaluateHeight(erosionBuffer[z * WorldConfig::CHUNK_SIZE + x]);
            float peaksAndValleysValue = peaksAndValleysSpline.evaluateHeight(peaksAndValleysBuffer[z * WorldConfig::CHUNK_SIZE + x]);
            //std::cout << normalizedNoise * maxHeight << " ";
            int height = continentalnessValue + (erosionValue * peaksAndValleysValue);
            height = std::clamp(height, 1, WorldConfig::MAX_HEIGHT - 1);
            int fillHeight = std::max(height, waterLevel);
            for (int y = 0; y <= fillHeight; y++){
                int chunkY = y / WorldConfig::CHUNK_SIZE;
                int localY = y % WorldConfig::CHUNK_SIZE;
                chunkColumn.blocks[x][y][z] = getBlockFromHeight(y, height);
            }
            //chunkColumn.Blocks[x][0][z] = 1;
            //chunkColumn.Blocks[x][63][z] = 2;
            //chunkColumn.Blocks[x][WorldConfig::MAX_HEIGHT-1][z] = 1;
        }
        //std::cout << std::endl;
    }

    {
        std::lock_guard<std::mutex> lock(chunksToSendMutex);
        chunksToSend.push(chunkToSend);
    }

}

void World::unloadChunkColumns(std::vector<glm::ivec2> &chunkColumnsToUnload) {

    std::vector<glm::ivec2> potentialChunkColumnsToUnload;

    for (auto& chunkColumnPos : chunkColumnsToUnload) {
        auto it = chunkColumns.find(chunkColumnPos);
        if (it != chunkColumns.end()) {
            it->second->playersLoadedChunkColumn--;
            if (it->second->playersLoadedChunkColumn <= 0) {
                potentialChunkColumnsToUnload.push_back(chunkColumnPos);
            }
        }
    }

    for (auto& chunkColumnPos : potentialChunkColumnsToUnload) {
        // must exists to be pushed to stack
        if (chunkColumns[chunkColumnPos]->playersLoadedChunkColumn <= 0) {
            chunkColumns.erase(chunkColumnPos);
        }
    }

}

void World::terrainGenerationWorkerThread() {

    int chunksGenerated = 0;
    while (terrainGenerationThreadsRunning) {

        ChunkToGenerate chunkToGenerate[chunkToGeneratePerThread];
        int chunksToGenerateCount = 0;

        {
            std::unique_lock<std::mutex> lock(chunksToGenerateMutex);

            chunksToGenerateCV.wait(lock, [this] { 
                return !chunksToGenerate.empty() || !terrainGenerationThreadsRunning;
            });

            if (!terrainGenerationThreadsRunning) {
                break; // Exit the thread if the flag is set to false
            }

            while (chunksToGenerateCount < chunkToGeneratePerThread && !chunksToGenerate.empty()) {
                chunkToGenerate[chunksToGenerateCount] = chunksToGenerate.front();
                chunksToGenerate.pop();
                chunksToGenerateCount++;
            }
        }


        ChunkPacket chunkPackets[chunkToGeneratePerThread][WorldConfig::CHUNK_COLUMN_HEIGHT];
        for (int i = 0; i < chunksToGenerateCount; i++){
            generateTerrain({chunkToGenerate[i].x, chunkToGenerate[i].z}, chunkToGenerate[i].peer);
        }

    }

}

Block World::getBlockAt(const glm::ivec3 &position) const {

    int chunkColumnX = std::floor(static_cast<float>(position.x) / WorldConfig::CHUNK_SIZE);
    int chunkColumnZ = std::floor(static_cast<float>(position.z) / WorldConfig::CHUNK_SIZE);
    auto it = chunkColumns.find({chunkColumnX, chunkColumnZ});
    if (it == chunkColumns.end()) {
        return Block::AIR; // Chunk column not loaded
    }
    if (position.y < 0 || position.y >= WorldConfig::CHUNK_SIZE * WorldConfig::CHUNK_COLUMN_HEIGHT) {
        return Block::AIR; // Y position out of bounds
    }
    int chunkPosX = position.x - chunkColumnX * WorldConfig::CHUNK_SIZE;
    int chunkPosZ = position.z - chunkColumnZ * WorldConfig::CHUNK_SIZE;
    return it->second->blocks[chunkPosX][position.y][chunkPosZ];

}

void World::updateBlock(glm::ivec3 position, Block block) {

    int chunkColumnX = std::floor(static_cast<float>(position.x) / WorldConfig::CHUNK_SIZE);
    int chunkColumnZ = std::floor(static_cast<float>(position.z) / WorldConfig::CHUNK_SIZE);

    auto it = chunkColumns.find({chunkColumnX, chunkColumnZ});
    if (it == chunkColumns.end()) {
        return; // Chunk column not loaded
    }
    if (position.y < 0 || position.y >= WorldConfig::CHUNK_SIZE * WorldConfig::CHUNK_COLUMN_HEIGHT) {
        return; // Y position out of bounds
    }
    int chunkPosX = position.x - chunkColumnX * WorldConfig::CHUNK_SIZE;
    int chunkPosZ = position.z - chunkColumnZ * WorldConfig::CHUNK_SIZE;
    it->second->blocks[chunkPosX][position.y][chunkPosZ] = block;

}

World::~World() {

}