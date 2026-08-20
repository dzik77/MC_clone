#pragma once
#include <Block.hpp>
#include <WorldConfig.hpp>
#include <cstdint>
#include <cstddef>

// Shared wire codec for a single vertical chunk (CHUNK_SIZE x CHUNK_SIZE x CHUNK_SIZE blocks)
// within a chunk column. Used by the server to compress chunks before sending and by the
// client to decompress them on receipt, so both sides must stay in sync.
//
// Body layout:
//   uint8_t wholeChunkUniform
//   if wholeChunkUniform: Block value                              (whole chunk is one block)
//   else, for each of CHUNK_SIZE horizontal layers (bottom to top):
//       uint8_t layerUniform
//       if layerUniform: Block value                                (layer is one block)
//       else: Block raw[CHUNK_SIZE * CHUNK_SIZE]                     (x-major, z-minor)
namespace ChunkCompression {

    constexpr int BLOCKS_PER_LAYER = WorldConfig::CHUNK_SIZE * WorldConfig::CHUNK_SIZE;
    // Worst case: 1 whole-chunk flag byte + per layer (1 flag byte + a full raw layer)
    constexpr size_t MAX_BODY_SIZE = 1 + WorldConfig::CHUNK_SIZE * (1 + BLOCKS_PER_LAYER);

    // columnBlocks is laid out [x][y][z] with y spanning the whole column
    // (WorldConfig::CHUNK_SIZE * WorldConfig::CHUNK_COLUMN_HEIGHT); chunkY selects which
    // CHUNK_SIZE-tall vertical slice (0..CHUNK_COLUMN_HEIGHT-1) to encode.
    // Writes the compressed body for that one chunk into out (must have room for at least
    // MAX_BODY_SIZE bytes) and returns the number of bytes actually written.
    inline size_t encodeChunk(
        const Block columnBlocks[WorldConfig::CHUNK_SIZE][WorldConfig::CHUNK_SIZE * WorldConfig::CHUNK_COLUMN_HEIGHT][WorldConfig::CHUNK_SIZE],
        int chunkY, uint8_t* out)
    {
        const int yBase = chunkY * WorldConfig::CHUNK_SIZE;
        size_t offset = 0;

        const Block chunkFirst = columnBlocks[0][yBase][0];
        bool wholeChunkUniform = true;
        for (int x = 0; x < WorldConfig::CHUNK_SIZE && wholeChunkUniform; x++) {
            for (int ly = 0; ly < WorldConfig::CHUNK_SIZE && wholeChunkUniform; ly++) {
                for (int z = 0; z < WorldConfig::CHUNK_SIZE; z++) {
                    if (columnBlocks[x][yBase + ly][z] != chunkFirst) {
                        wholeChunkUniform = false;
                        break;
                    }
                }
            }
        }

        if (wholeChunkUniform) {
            out[offset++] = 1;
            out[offset++] = static_cast<uint8_t>(chunkFirst);
            return offset;
        }
        out[offset++] = 0;

        for (int ly = 0; ly < WorldConfig::CHUNK_SIZE; ly++) {
            const int y = yBase + ly;
            const Block layerFirst = columnBlocks[0][y][0];
            bool layerUniform = true;
            for (int x = 0; x < WorldConfig::CHUNK_SIZE && layerUniform; x++) {
                for (int z = 0; z < WorldConfig::CHUNK_SIZE; z++) {
                    if (columnBlocks[x][y][z] != layerFirst) {
                        layerUniform = false;
                        break;
                    }
                }
            }

            if (layerUniform) {
                out[offset++] = 1;
                out[offset++] = static_cast<uint8_t>(layerFirst);
            } else {
                out[offset++] = 0;
                for (int x = 0; x < WorldConfig::CHUNK_SIZE; x++) {
                    for (int z = 0; z < WorldConfig::CHUNK_SIZE; z++) {
                        out[offset++] = static_cast<uint8_t>(columnBlocks[x][y][z]);
                    }
                }
            }
        }

        return offset;
    }

    // Decodes a compressed chunk body into a flat CHUNK_SIZE^3 buffer laid out as
    // outFlatBlocks[x * BLOCKS_PER_LAYER + localY * CHUNK_SIZE + z], matching ChunkPacket::chunkBlocks.
    // Bails out early on a truncated/malformed body instead of reading past inLength.
    inline void decodeChunk(const uint8_t* in, size_t inLength, Block* outFlatBlocks) {
        size_t offset = 0;
        if (offset >= inLength) return;
        const uint8_t wholeChunkUniform = in[offset++];

        if (wholeChunkUniform) {
            if (offset >= inLength) return;
            const Block value = static_cast<Block>(in[offset++]);
            for (int i = 0; i < WorldConfig::CHUNK_SIZE * BLOCKS_PER_LAYER; i++) {
                outFlatBlocks[i] = value;
            }
            return;
        }

        for (int ly = 0; ly < WorldConfig::CHUNK_SIZE; ly++) {
            if (offset >= inLength) return;
            const uint8_t layerUniform = in[offset++];

            if (layerUniform) {
                if (offset >= inLength) return;
                const Block value = static_cast<Block>(in[offset++]);
                for (int x = 0; x < WorldConfig::CHUNK_SIZE; x++) {
                    for (int z = 0; z < WorldConfig::CHUNK_SIZE; z++) {
                        outFlatBlocks[x * BLOCKS_PER_LAYER + ly * WorldConfig::CHUNK_SIZE + z] = value;
                    }
                }
            } else {
                for (int x = 0; x < WorldConfig::CHUNK_SIZE; x++) {
                    for (int z = 0; z < WorldConfig::CHUNK_SIZE; z++) {
                        if (offset >= inLength) return;
                        outFlatBlocks[x * BLOCKS_PER_LAYER + ly * WorldConfig::CHUNK_SIZE + z] = static_cast<Block>(in[offset++]);
                    }
                }
            }
        }
    }

}
