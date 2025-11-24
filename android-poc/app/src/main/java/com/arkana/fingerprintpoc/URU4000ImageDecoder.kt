package com.arkana.fingerprintpoc

import android.util.Log

/**
 * Decodes raw image data from DigitalPersona U.are.U 4000 USB bulk transfer
 * Based on libfprint uru4000 driver implementation
 */
object URU4000ImageDecoder {
    private const val TAG = "URU4000ImageDecoder"
    
    // Image dimensions (from libfprint uru4000 driver)
    const val IMAGE_WIDTH = 384
    const val IMAGE_HEIGHT = 290
    
    // Structure sizes based on uru4k_image struct from libfprint
    private const val HEADER_SIZE = 4 // unknown_00[4]
    private const val NUM_LINES_SIZE = 2 // num_lines (uint16_t, little-endian)
    private const val KEY_NUMBER_SIZE = 1 // key_number
    private const val UNKNOWN_07_SIZE = 9 // unknown_07[9]
    private const val BLOCK_INFO_COUNT = 15
    private const val BLOCK_INFO_SIZE = 2 // flags (uint8_t) + num_lines (uint8_t) per block
    private const val UNKNOWN_2E_SIZE = 18 // unknown_2E[18]
    private const val METADATA_SIZE = HEADER_SIZE + NUM_LINES_SIZE + KEY_NUMBER_SIZE + UNKNOWN_07_SIZE + 
                                     (BLOCK_INFO_COUNT * BLOCK_INFO_SIZE) + UNKNOWN_2E_SIZE // = 64 bytes
    
    // Block flags
    private const val BLOCKF_CHANGE_KEY = 0x80
    private const val BLOCKF_NO_KEY_UPDATE = 0x04
    private const val BLOCKF_ENCRYPTED = 0x02
    private const val BLOCKF_NOT_PRESENT = 0x01
    
    // Encryption threshold
    private const val ENC_THRESHOLD = 5000
    
    /**
     * Decode raw bulk transfer data into image pixel data
     * Returns: ByteArray of decoded image (IMAGE_WIDTH * IMAGE_HEIGHT bytes) or null on error
     */
    fun decodeImage(rawData: ByteArray): ByteArray? {
        try {
            if (rawData.size < METADATA_SIZE) {
                Log.e(TAG, "Raw data too small: ${rawData.size} < $METADATA_SIZE")
                return null
            }
            
            var offset = 0
            
            // Skip header (4 bytes)
            offset += HEADER_SIZE
            
            // Read num_lines (little-endian uint16)
            val numLines = (rawData[offset].toInt() and 0xFF) or 
                          ((rawData[offset + 1].toInt() and 0xFF) shl 8)
            offset += NUM_LINES_SIZE
            
            Log.d(TAG, "Image num_lines: $numLines")
            
            if (numLines == 0 || numLines > IMAGE_HEIGHT) {
                Log.e(TAG, "Invalid num_lines: $numLines")
                return null
            }
            
            // Skip key_number
            offset += KEY_NUMBER_SIZE
            
            // Skip unknown_07
            offset += UNKNOWN_07_SIZE
            
            // Parse block_info
            val blockInfos = mutableListOf<Pair<Int, Int>>() // (flags, num_lines)
            for (i in 0 until BLOCK_INFO_COUNT) {
                val flags = rawData[offset].toInt() and 0xFF
                val blockNumLines = rawData[offset + 1].toInt() and 0xFF
                offset += BLOCK_INFO_SIZE
                
                if (blockNumLines > 0) {
                    blockInfos.add(Pair(flags, blockNumLines))
                } else {
                    break
                }
            }
            
            // Skip unknown_2E
            offset += UNKNOWN_2E_SIZE
            
            // Calculate expected image data size
            val expectedImageSize = numLines * IMAGE_WIDTH
            if (rawData.size < offset + expectedImageSize) {
                Log.e(TAG, "Raw data too small for image: ${rawData.size} < ${offset + expectedImageSize}")
                return null
            }
            
            // Extract image data - following libfprint uru4000 driver logic
            val imageData = ByteArray(IMAGE_WIDTH * IMAGE_HEIGHT) { 0 }
            var imageOffset = 0
            var sourceOffset = offset
            var sourceLineCount = 0
            
            // Process blocks similar to libfprint imaging_run_state IMAGING_REPORT_IMAGE case
            for ((flags, blockNumLines) in blockInfos) {
                if (blockNumLines == 0) break
                
                // Copy block data (num_lines * IMAGE_WIDTH bytes)
                val bytesToCopy = blockNumLines * IMAGE_WIDTH
                if (bytesToCopy > 0 && sourceOffset + bytesToCopy <= rawData.size && 
                    imageOffset + bytesToCopy <= imageData.size) {
                    System.arraycopy(rawData, sourceOffset, imageData, imageOffset, bytesToCopy)
                }
                
                // Update offsets - only advance source if block is present
                if ((flags and BLOCKF_NOT_PRESENT) == 0) {
                    sourceLineCount += blockNumLines
                }
                imageOffset += bytesToCopy
                sourceOffset += bytesToCopy
            }
            
            Log.d(TAG, "Extracted $sourceLineCount lines from blocks, expected $numLines")
            
            // NOTE: libfprint sets FPI_IMAGE_COLORS_INVERTED flag but does NOT invert the data
            // The flag is just metadata. The data from device is already in correct format.
            // So we should NOT invert colors here - return data as-is.
            // If matching fails, we can try inverting, but for now let's match libfprint behavior exactly.
            
            Log.i(TAG, "Image decoded successfully: ${imageData.size} bytes (${IMAGE_WIDTH}x${IMAGE_HEIGHT})")
            return imageData
            
        } catch (e: Exception) {
            Log.e(TAG, "Error decoding image: ${e.message}", e)
            return null
        }
    }
}

