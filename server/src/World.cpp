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

    vegetation = FastNoise::New<FastNoise::Simplex>();
    vegetation->SetScale(1000.f);

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

void World::sentAndLoadChunkColumn(glm::ivec2 chunkColumnPosition, ENetPeer* peer) {

    ChunkToGenerate chunkToGenerate;
    chunkToGenerate.x = chunkColumnPosition.x;
    chunkToGenerate.z = chunkColumnPosition.y;
    chunkToGenerate.peer = peer;
    if (chunkColumns.find(chunkColumnPosition) == chunkColumns.end()) {
        std::lock_guard<std::mutex> lock(chunksToGenerateMutex);
        chunksToGenerate.push(chunkToGenerate);
        chunksToGenerateCV.notify_one();
    } else {
        // If the chunk column already exists, we still need to send it to the player
        ChunkToSend chunkToSend;
        chunkToSend.chunkPacket = chunkColumns[chunkColumnPosition];
        chunkToSend.upload = false;
        chunkToSend.peer = peer;
        std::lock_guard<std::mutex> lock(chunksToSendMutex);
        chunksToSend.push(chunkToSend);
    }

}

ChunkSendBuffer* World::chunkColumnToPacket(ChunkColumn& chunkColumn){
    ChunkSendBuffer* buffers = new ChunkSendBuffer[WorldConfig::CHUNK_COLUMN_HEIGHT];

    for (int y = 0; y < WorldConfig::CHUNK_COLUMN_HEIGHT; y++){
        ChunkPacketHeader header;
        header.x = chunkColumn.position.x;
        header.z = chunkColumn.position.y;
        header.y = y;
        std::memcpy(buffers[y].data, &header, sizeof(header));

        size_t bodyLength = ChunkCompression::encodeChunk(chunkColumn.blocks, y, buffers[y].data + sizeof(header));
        buffers[y].length = sizeof(header) + bodyLength;
    }

    return buffers;
}

void World::generateTerrain(glm::ivec2 chunkColumnPosition, ENetPeer* peer) {

    // base terrain pass

    // Noise is sampled over a (CHUNK_SIZE + 2*borderMargin) square centered on the column so a
    // later decoration pass can read past the column's edge without needing neighboring columns.
    constexpr int paddedSize = WorldConfig::CHUNK_SIZE + 2 * borderMargin;
    int noiseOriginX = chunkColumnPosition.x * WorldConfig::CHUNK_SIZE - borderMargin;
    int noiseOriginZ = chunkColumnPosition.y * WorldConfig::CHUNK_SIZE - borderMargin;

    // Generate noise values for the chunk column (SIMD optimized)
    std::vector<float> continentalnessBuffer(paddedSize * paddedSize), erosionBuffer(paddedSize * paddedSize), peaksAndValleysBuffer(paddedSize * paddedSize), vegetationBuffer(paddedSize * paddedSize);
    continentalness->GenUniformGrid2D(continentalnessBuffer.data(), noiseOriginX, noiseOriginZ, paddedSize, paddedSize, 1.f, 1.f, seed);
    erosionFractal->GenUniformGrid2D(erosionBuffer.data(), noiseOriginX, noiseOriginZ, paddedSize, paddedSize, 1.f, 1.f, seed);
    peaksAndValleysFractal->GenUniformGrid2D(peaksAndValleysBuffer.data(), noiseOriginX, noiseOriginZ, paddedSize, paddedSize, 1.f, 1.f, seed);
    vegetation->GenUniformGrid2D(vegetationBuffer.data(), noiseOriginX, noiseOriginZ, paddedSize, paddedSize, 1.f, 1.f, seed);

    
    ChunkToSend chunkToSend;
    chunkToSend.chunkPacket = std::make_shared<ChunkColumn>();
    auto& chunkColumn = *chunkToSend.chunkPacket;
    chunkColumn.position = chunkColumnPosition;
    chunkToSend.upload = true;
    chunkToSend.peer = peer;

    // Combine the noise values to determine the final height of each column in the chunk
    for (int x = 0; x < WorldConfig::CHUNK_SIZE; x++){
        for (int z = 0; z < WorldConfig::CHUNK_SIZE; z++){
            int bufferIndex = (z + borderMargin) * paddedSize + (x + borderMargin);
            float continentalnessValue = continentalnessSpline.evaluateHeight(continentalnessBuffer[bufferIndex]);
            float erosionValue = erosionSpline.evaluateHeight(erosionBuffer[bufferIndex]);
            float peaksAndValleysValue = peaksAndValleysSpline.evaluateHeight(peaksAndValleysBuffer[bufferIndex]);
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

    // decoration pass 

    auto setBlockIfAir = [&](int x, int y, int z, Block block) {
        if (x < borderMargin || x >= paddedSize - borderMargin || z < borderMargin || z >= paddedSize - borderMargin || y < 0 || y >= WorldConfig::CHUNK_COLUMN_HEIGHT* WorldConfig::CHUNK_SIZE) return;
        if (chunkColumn.blocks[x - borderMargin][y][z - borderMargin] == Block::AIR) {
            chunkColumn.blocks[x - borderMargin][y][z - borderMargin] = block;
        }
    };

    int startx = chunkColumnPosition.x * WorldConfig::CHUNK_SIZE - borderMargin;
    int endx = (chunkColumnPosition.x+1) * WorldConfig::CHUNK_SIZE + borderMargin;
    int startz = chunkColumnPosition.y * WorldConfig::CHUNK_SIZE - borderMargin;
    int endz = (chunkColumnPosition.y+1) * WorldConfig::CHUNK_SIZE + borderMargin;
    auto floorToMultiple = [](int v, int m) { return v - (((v % m) + m) % m); };
    for (int x = floorToMultiple(startx, treeMargin); x < endx; x+= treeMargin){
        for (int z = floorToMultiple(startz, treeMargin); z < endz; z+= treeMargin){

            uint32_t hash = hashCell(x, z);
            int hashX = hash & (0xFFFF);
            int hashZ = (hash >> 16) & (0xFFFF);
            int worldX = hashX % treeMargin + x;
            int worldZ = hashZ % treeMargin + z;
            if (worldX < startx || worldX >= endx || worldZ < startz || worldZ >= endz) continue;

            int bufferIndex = ((worldZ - startz) * paddedSize) + (worldX - startx);
            float vegetationValue = vegetationBuffer[bufferIndex];
            //std::cout << normalizedNoise * maxHeight << " ";

            float continentalnessValue = continentalnessSpline.evaluateHeight(continentalnessBuffer[bufferIndex]);
            float erosionValue = erosionSpline.evaluateHeight(erosionBuffer[bufferIndex]);
            float peaksAndValleysValue = peaksAndValleysSpline.evaluateHeight(peaksAndValleysBuffer[bufferIndex]);

            int height = continentalnessValue + (erosionValue * peaksAndValleysValue);
            height = std::clamp(height, 1, WorldConfig::MAX_HEIGHT - 1);

            if (vegetationValue > 0.5f && height > waterLevel){

                int localTreeX = worldX - startx;
                int localTreeZ = worldZ - startz;

                for (int y = height + 1; y <= height + 5; y++){
                    setBlockIfAir(localTreeX, y, localTreeZ, Block::LOG);
                }
                for (int y = height + 3; y <= height + 4; y++){
                    for (int dx = -2; dx <= 2; dx++){
                        for (int dz = -2; dz <= 2; dz++){
                            if (dx*dx+dz*dz <= 4){
                                setBlockIfAir(localTreeX + dx, y, localTreeZ + dz, Block::LEAVES);
                            }
                        }
                    }
                }
                for (int dx = -1; dx <= 1; dx++){
                    for (int dz = -1; dz <= 1; dz++){
                        if (dx*dx+dz*dz <= 1){
                            setBlockIfAir(localTreeX + dx, height + 5, localTreeZ + dz, Block::LEAVES);
                        }
                    }
                }
                setBlockIfAir(localTreeX, height + 6, localTreeZ, Block::LEAVES);
               

            }
        }
    }

    // sending
    {
        std::lock_guard<std::mutex> lock(chunksToSendMutex);
        chunksToSend.push(chunkToSend);
    }

}

void World::unloadChunkColumns(std::vector<glm::ivec2> &chunkColumnsToUnload) {

    for (auto& chunkColumnPos : chunkColumnsToUnload) {
        chunkColumnPlayersLoadedCount[chunkColumnPos]--;
        if (chunkColumnPlayersLoadedCount[chunkColumnPos] <= 0) {
            chunkColumnPlayersLoadedCount.erase(chunkColumnPos);
            if (chunkColumns.find(chunkColumnPos) != chunkColumns.end()) {
                chunkColumns.erase(chunkColumnPos);
            }
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