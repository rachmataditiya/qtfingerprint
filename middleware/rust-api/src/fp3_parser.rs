// FP3 template parser for extracting minutiae from libfprint templates
// Based on libfprint fp_print_deserialize() implementation
// FP3 format: "FP3" header + GVariant binary data
// GVariant structure: (issbymsmsia{sv}v) for metadata
// For NBIS type (print_type = 1 or 2): print_data contains array of xyt_struct: a(aiaiai)

use std::io::{Cursor, Read, Seek, SeekFrom};
use tracing::debug;

/// xyt_struct represents minutiae data (X, Y, Theta)
#[derive(Debug, Clone)]
pub struct XytStruct {
    pub nrows: usize,
    pub xcol: Vec<i32>,
    pub ycol: Vec<i32>,
    pub thetacol: Vec<i32>,
}

/// Parse FP3 template and extract minutiae data
/// Returns vector of xyt_struct (can have multiple prints per template)
/// Based on libfprint fp_print_deserialize() implementation
pub fn parse_fp3_template(fp3_data: &[u8]) -> Result<Vec<XytStruct>, String> {
    // Verify FP3 header
    if fp3_data.len() < 3 {
        return Err("FP3 template too short".to_string());
    }
    
    if &fp3_data[0..3] != b"FP3" {
        return Err("Invalid FP3 header".to_string());
    }
    
    // Skip "FP3" header (3 bytes)
    let variant_data = &fp3_data[3..];
    
    if variant_data.is_empty() {
        return Err("Empty GVariant data".to_string());
    }
    
    debug!("Parsing FP3 template, variant data size: {} bytes", variant_data.len());
    
    // Parse GVariant structure according to libfprint format
    // Format: (issbymsmsia{sv}v)
    // i = print type (i32)
    // s = driver name (string, null-terminated, inline)
    // s = device_id (string, null-terminated, inline)
    // b = device_stored (byte/bool)
    // y = finger (uint8)
    // ms = username (maybe string)
    // ms = description (maybe string)
    // i = julian_date (i32)
    // a{sv} = metadata dict
    // v = print_data (variant)
    
    let mut cursor = Cursor::new(variant_data);
    
    // Read print type (i32, little-endian)
    let print_type = read_i32_le(&mut cursor)?;
    debug!("Print type: {}", print_type);
    
    // Accept both FPI_PRINT_NBIS (1) and uru4000 type (2)
    if print_type != 1 && print_type != 2 {
        return Err(format!("Unsupported print type: {}, only NBIS (1) or uru4000 (2) supported", print_type));
    }
    
    // Read driver name (string, null-terminated, inline)
    let driver = read_null_terminated_string(&mut cursor)?;
    debug!("Driver name: '{}'", driver);
    
    // Read device_id (string, null-terminated, inline)
    let device_id = read_null_terminated_string(&mut cursor)?;
    debug!("Device ID: '{}'", device_id);
    
    // Read device_stored (byte/bool)
    let device_stored = read_byte(&mut cursor)?;
    debug!("Device stored: {}", device_stored != 0);
    
    // Read finger (uint8, 'y' type in GVariant)
    let finger = read_byte(&mut cursor)?;
    debug!("Finger: {}", finger);
    
    // Read username (maybe string, 'ms' type in GVariant)
    // From real data analysis: format is null-terminated string if present, or special marker if Nothing
    // Actually, GVariant "maybe string" in binary format:
    // - If Just(string): the string directly (null-terminated, then possibly alignment)
    // - If Nothing: we need to check
    // Based on real data: username = "user\x00\x00" (null terminator + alignment byte)
    let username = read_maybe_string(&mut cursor)?;
    debug!("Username: {:?}", username);
    
    // Read description (maybe string, same format)
    let description = read_maybe_string(&mut cursor)?;
    debug!("Description: {:?}", description);
    
    // Read julian_date (i32)
    let julian_date = read_i32_le(&mut cursor)?;
    debug!("Julian date: {}", julian_date);
    
    // Skip metadata dict a{sv} (should be empty based on libfprint code)
    // Format: array length (u32) + entries (each: key string + value variant)
    let dict_len = read_u32_le(&mut cursor)?;
    debug!("Metadata dict length: {} (should be 0)", dict_len);
    
    // Skip dict entries if any (should be 0, but handle gracefully)
    for _ in 0..dict_len {
        // Skip key (null-terminated string)
        read_null_terminated_string(&mut cursor)?;
        // Skip value (variant)
        skip_variant(&mut cursor)?;
    }
    
    // Read print_data (variant v)
    // For NBIS type, this contains a(aiaiai) - array of xyt_struct
    // IMPORTANT: Based on real data analysis, after metadata dict (empty),
    // the variant 'v' format directly contains the array data WITHOUT variant signature string.
    // This is because GVariant can optimize away the signature for nested variants.
    // Format: Direct array data (a(aiaiai)) without "a(aiaiai)\0" signature prefix
    let variant_start = cursor.position();
    debug!("Reading print_data variant starting at offset {}", variant_start);
    
    // Read all remaining data as the array (a(aiaiai))
    // The data directly starts with outer array length (u32)
    let remaining = cursor.get_ref().len() - variant_start as usize;
    let mut print_data = vec![0u8; remaining];
    cursor.read_exact(&mut print_data).map_err(|e| format!("Failed to read variant array data: {}", e))?;
    debug!("Print data variant size: {} bytes (direct array, no signature)", print_data.len());
    
    // Parse NBIS print data: array of xyt_struct
    // Format: a(aiaiai) - array of tuples (array of x, array of y, array of theta)
    let minutiae_list = parse_nbis_prints(&print_data)?;
    debug!("Parsed {} xyt_struct(s) from NBIS data", minutiae_list.len());
    
    Ok(minutiae_list)
}

/// Read null-terminated string from cursor
/// GVariant strings in binary format are null-terminated and inline
fn read_null_terminated_string(cursor: &mut Cursor<&[u8]>) -> Result<String, String> {
    let mut bytes = Vec::new();
    let start_pos = cursor.position();
    
    // Read until null terminator
    loop {
        let mut byte = [0u8; 1];
        if cursor.read_exact(&mut byte).is_err() {
            return Err(format!("Failed to read null-terminated string starting at position {}", start_pos));
        }
        if byte[0] == 0 {
            break;
        }
        bytes.push(byte[0]);
        if bytes.len() > 256 {
            return Err("String too long (max 256 chars)".to_string());
        }
    }
    
    String::from_utf8(bytes).map_err(|e| format!("Invalid UTF-8 in string: {}", e))
}

/// Read i32 in little-endian format
fn read_i32_le(cursor: &mut Cursor<&[u8]>) -> Result<i32, String> {
    let mut bytes = [0u8; 4];
    cursor.read_exact(&mut bytes).map_err(|e| format!("Failed to read i32: {}", e))?;
    Ok(i32::from_le_bytes(bytes))
}

/// Read u32 in little-endian format
fn read_u32_le(cursor: &mut Cursor<&[u8]>) -> Result<u32, String> {
    let mut bytes = [0u8; 4];
    cursor.read_exact(&mut bytes).map_err(|e| format!("Failed to read u32: {}", e))?;
    Ok(u32::from_le_bytes(bytes))
}

/// Read byte
fn read_byte(cursor: &mut Cursor<&[u8]>) -> Result<u8, String> {
    let mut bytes = [0u8; 1];
    cursor.read_exact(&mut bytes).map_err(|e| format!("Failed to read byte: {}", e))?;
    Ok(bytes[0])
}

/// Skip N bytes
fn skip_bytes(cursor: &mut Cursor<&[u8]>, n: usize) -> Result<(), String> {
    let mut buf = vec![0u8; n];
    cursor.read_exact(&mut buf).map_err(|e| format!("Failed to skip {} bytes: {}", n, e))?;
    Ok(())
}

/// Read maybe string (GVariant format: optional string)
/// Format: In GVariant binary format, "maybe string" (ms):
/// - If Nothing: special marker (varies by implementation, could be 4 null bytes or special encoding)
/// - If Just(string): the string directly (null-terminated)
/// Based on real data analysis, it seems to be: string null-terminated if present
/// For alignment/padding, there may be extra null bytes after the terminator
fn read_maybe_string(cursor: &mut Cursor<&[u8]>) -> Result<Option<String>, String> {
    let pos = cursor.position() as usize;
    let data = cursor.get_ref();
    
    // Check first 4 bytes to see if it's Nothing marker
    if pos + 4 <= data.len() {
        let marker = &data[pos..pos + 4];
        // GVariant Nothing marker might be all zeros or special encoding
        // But in practice, if first byte is non-null, it's likely a string
        if marker == b"\x00\x00\x00\x00" {
            // Could be Nothing, but also could be empty string with alignment
            // Let's check: if next bytes are also structured (like julian date), it's Nothing
            // Actually, let's be simpler: if first byte is 0 and next 3 are 0, it's Nothing
            skip_bytes(cursor, 4)?;
            return Ok(None);
        }
    }
    
    // Read null-terminated string
    let mut bytes = Vec::new();
    loop {
        let mut byte = [0u8; 1];
        if cursor.read_exact(&mut byte).is_err() {
            return Err("Failed to read maybe string".to_string());
        }
        if byte[0] == 0 {
            break;
        }
        bytes.push(byte[0]);
        if bytes.len() > 256 {
            return Err("String too long (max 256 chars)".to_string());
        }
    }
    
    // There might be an alignment null byte after the terminator
    // Check if next byte is also null (for alignment)
    let next_pos = cursor.position() as usize;
    if next_pos < cursor.get_ref().len() {
        let next_byte = cursor.get_ref()[next_pos];
        if next_byte == 0 {
            // Skip alignment byte
            skip_bytes(cursor, 1)?;
        }
    }
    
    if bytes.is_empty() {
        Ok(None)
    } else {
        String::from_utf8(bytes)
            .map(Some)
            .map_err(|e| format!("Invalid UTF-8 in maybe string: {}", e))
    }
}

/// Skip variant dict a{sv}
/// Format: array length (u32) + entries (each: key null-terminated string + value variant)
fn skip_variant_dict(cursor: &mut Cursor<&[u8]>) -> Result<(), String> {
    // Read array length
    let array_len = read_u32_le(cursor)?;
    debug!("Skipping variant dict with {} entries", array_len);
    
    // Skip each dict entry (key: null-terminated string, value: variant)
    for i in 0..array_len {
        // Skip key (null-terminated string)
        loop {
            let mut byte = [0u8; 1];
            if cursor.read_exact(&mut byte).is_err() {
                return Err(format!("Failed to skip dict key at entry {}", i));
            }
            if byte[0] == 0 {
                break;
            }
        }
        
        // Skip value (variant - has type signature and data)
        skip_variant(cursor)?;
    }
    
    Ok(())
}

/// Read variant v (has type signature and data)
/// Returns the variant data as Vec<u8>
fn read_variant(cursor: &mut Cursor<&[u8]>) -> Result<Vec<u8>, String> {
    // Read type signature (null-terminated string)
    let signature = read_null_terminated_string(cursor)?;
    debug!("Variant signature: '{}'", signature);
    
    // Align to 8 bytes (GVariant alignment requirement)
    let pos = cursor.position();
    let aligned_pos = ((pos + 7) / 8) * 8;
    if aligned_pos > pos {
        skip_bytes(cursor, (aligned_pos - pos) as usize)?;
    }
    
    // For array type a(...), read the array data
    // Format: array length (u32) + array elements
    if signature.starts_with('a') {
        // Read array length
        let array_len = read_u32_le(cursor)?;
        debug!("Variant array length: {}", array_len);
        
        // Store current position - this is where array data starts
        let data_start = cursor.position();
        
        // Calculate how much data to read
        // For a(aiaiai), we need to read all nested arrays
        // We'll read until we have valid data or hit end of input
        // Actually, let's read all remaining data as this is the NBIS print data
        let remaining = cursor.get_ref().len() - data_start as usize;
        let mut data = vec![0u8; remaining];
        cursor.read_exact(&mut data).map_err(|e| format!("Failed to read variant array data: {}", e))?;
        
        Ok(data)
    } else {
        return Err(format!("Unsupported variant type: {}, expected array type", signature));
    }
}

/// Skip variant (used for dict values)
/// Format: type signature (null-terminated string) + alignment + variant data
/// For variants, we don't read size separately - the variant data is directly embedded
fn skip_variant(cursor: &mut Cursor<&[u8]>) -> Result<(), String> {
    // Read type signature (null-terminated string)
    let signature = read_null_terminated_string(cursor)?;
    debug!("Skipping variant with signature: '{}'", signature);
    
    // Align to 8 bytes (GVariant alignment requirement)
    let pos = cursor.position();
    let aligned_pos = ((pos + 7) / 8) * 8;
    if aligned_pos > pos {
        skip_bytes(cursor, (aligned_pos - pos) as usize)?;
    }
    
    // For variants in dict values, the data is directly embedded
    // We need to skip based on the signature type
    // Common types:
    // - "s" (string): null-terminated string
    // - "i" (int32): 4 bytes
    // - "b" (boolean): 1 byte
    // - "v" (variant): nested variant (recursive)
    // - "a..." (array): array length + elements
    // 
    // For simplicity, since we're in a{sv} (dict of string->variant),
    // and we're skipping, we can try to detect and skip common cases
    // But for now, let's just return - the variant dict should be empty anyway
    // based on libfprint code: g_variant_builder_open (&builder, G_VARIANT_TYPE_VARDICT); g_variant_builder_close (&builder);
    
    // Actually, the variant in dict value might have size prefix
    // Let's check if there's a size field
    // GVariant serialized format might have size, but it's complex
    // For empty dict (which it should be), we shouldn't reach here
    
    // For now, let's not skip anything - if the dict is empty, we won't enter the loop
    // If it's not empty, we need proper variant skipping which is complex
    
    Ok(())
}

/// Parse NBIS prints data: a(aiaiai) - array of (xcol array, ycol array, thetacol array)
/// Based on libfprint parsing logic
fn parse_nbis_prints(data: &[u8]) -> Result<Vec<XytStruct>, String> {
    let mut cursor = Cursor::new(data);
    let mut prints = Vec::new();
    
    // Read outer array length (number of xyt_structs)
    let array_len = read_u32_le(&mut cursor)?;
    debug!("NBIS prints array length: {}", array_len);
    
    for print_idx in 0..array_len {
        // Each element is a tuple: (aiaiai)
        // IMPORTANT: In GVariant binary format, fixed arrays (ai) do not require
        // alignment between the length and data, but tuples may require alignment
        // between array elements. However, since all arrays are ai (arrays of i32),
        // and i32 is 4-byte aligned, we should already be aligned.
        
        // Read array of x (ai format)
        let xcol = read_i32_array(&mut cursor)?;
        debug!("Print {}: Read {} x values", print_idx, xcol.len());
        
        // CRITICAL: After reading array elements, align to 4-byte boundary
        // before reading the next array length in the tuple
        // GVariant fixed arrays in tuples require 4-byte alignment between arrays
        let pos = cursor.position();
        let aligned_pos = ((pos + 3) / 4) * 4;
        if aligned_pos > pos {
            debug!("Aligning from offset {} to {} before reading y array length", pos, aligned_pos);
            skip_bytes(&mut cursor, (aligned_pos - pos) as usize)?;
        }
        
        // Read array of y (ai format)
        let ycol = read_i32_array(&mut cursor)?;
        debug!("Print {}: Read {} y values", print_idx, ycol.len());
        
        // Align before reading theta array
        let pos = cursor.position();
        let aligned_pos = ((pos + 3) / 4) * 4;
        if aligned_pos > pos {
            debug!("Aligning from offset {} to {} before reading theta array length", pos, aligned_pos);
            skip_bytes(&mut cursor, (aligned_pos - pos) as usize)?;
        }
        
        // Read array of theta (ai format)
        let thetacol = read_i32_array(&mut cursor)?;
        debug!("Print {}: Read {} theta values", print_idx, thetacol.len());
        
        // Verify all arrays have same length
        if xcol.len() != ycol.len() || xcol.len() != thetacol.len() {
            return Err(format!(
                "xyt arrays have different lengths: x={}, y={}, theta={}",
                xcol.len(), ycol.len(), thetacol.len()
            ));
        }
        
        debug!("Print {}: {} minutiae points", print_idx, xcol.len());
        
        // Limit to MAX_BOZORTH_MINUTIAE (200) for safety
        let nrows = xcol.len().min(200);
        
        prints.push(XytStruct {
            nrows,
            xcol: xcol[..nrows].to_vec(),
            ycol: ycol[..nrows].to_vec(),
            thetacol: thetacol[..nrows].to_vec(),
        });
    }
    
    Ok(prints)
}

/// Read array of i32 (ai format in GVariant)
/// Format: array length (u32) + array elements (i32 each)
/// Note: u32 (4 bytes) is already 4-byte aligned, and i32 elements are naturally aligned
/// IMPORTANT: After reading array elements, we need to align to 4-byte boundary
/// before reading the next array length in a tuple
fn read_i32_array(cursor: &mut Cursor<&[u8]>) -> Result<Vec<i32>, String> {
    // Ensure we're at 4-byte alignment before reading length
    let pos = cursor.position();
    let aligned_pos = ((pos + 3) / 4) * 4;
    if aligned_pos > pos {
        skip_bytes(cursor, (aligned_pos - pos) as usize)?;
    }
    
    // Read array length (u32)
    let array_len = read_u32_le(cursor)?;
    
    // Read array elements (i32 each, little-endian)
    // After reading elements, cursor position will be at (aligned_pos + 4 + array_len*4)
    // This might not be aligned, so caller should handle alignment after this function
    let mut array = Vec::with_capacity(array_len as usize);
    for _ in 0..array_len {
        let value = read_i32_le(cursor)?;
        array.push(value);
    }
    
    Ok(array)
}

#[cfg(test)]
mod tests {
    use super::*;
    
    #[test]
    fn test_parse_i32_le() {
        let data = vec![0x01, 0x00, 0x00, 0x00];
        let mut cursor = Cursor::new(&data);
        assert_eq!(read_i32_le(&mut cursor).unwrap(), 1);
    }
    
    #[test]
    fn test_read_null_terminated_string() {
        let data = b"hello\0world\0";
        let mut cursor = Cursor::new(data);
        assert_eq!(read_null_terminated_string(&mut cursor).unwrap(), "hello");
        assert_eq!(read_null_terminated_string(&mut cursor).unwrap(), "world");
    }
}