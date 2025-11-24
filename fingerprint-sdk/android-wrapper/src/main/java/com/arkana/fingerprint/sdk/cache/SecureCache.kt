package com.arkana.fingerprint.sdk.cache

import android.content.Context
import android.security.keystore.KeyGenParameterSpec
import android.security.keystore.KeyProperties
import com.arkana.fingerprint.sdk.util.FingerError
import java.security.KeyStore
import javax.crypto.Cipher
import javax.crypto.KeyGenerator
import javax.crypto.SecretKey
import javax.crypto.spec.GCMParameterSpec
import java.util.Base64

/**
 * Secure cache for fingerprint templates.
 * 
 * Uses:
 * - AES-256-GCM encryption
 * - Android Keystore for key management
 * - Binary encrypted format
 */
class SecureCache(private val context: Context, private val enabled: Boolean) {
    private val keyStore: KeyStore = KeyStore.getInstance("AndroidKeyStore").apply {
        load(null)
    }
    private val keyAlias = "arkana_fingerprint_key"
    private val sharedPrefs = context.getSharedPreferences("arkana_fp_cache", Context.MODE_PRIVATE)

    init {
        if (enabled) {
            ensureKeyExists()
        }
    }

    /**
     * Store template in cache (encrypted).
     */
    fun store(userId: Int, template: ByteArray) {
        if (!enabled) return

        try {
            val encrypted = encrypt(template)
            val encryptedBase64 = Base64.getEncoder().encodeToString(encrypted)
            sharedPrefs.edit()
                .putString("template_$userId", encryptedBase64)
                .apply()
        } catch (e: Exception) {
            // Ignore cache errors
        }
    }

    /**
     * Load template from cache (decrypted).
     */
    fun load(userId: Int): ByteArray? {
        if (!enabled) return null

        return try {
            val encryptedBase64 = sharedPrefs.getString("template_$userId", null)
                ?: return null
            val encrypted = Base64.getDecoder().decode(encryptedBase64)
            decrypt(encrypted)
        } catch (e: Exception) {
            null
        }
    }

    /**
     * Clear cache for user.
     */
    fun clear(userId: Int) {
        sharedPrefs.edit()
            .remove("template_$userId")
            .apply()
    }

    /**
     * Clear all cache.
     */
    fun clearAll() {
        sharedPrefs.edit().clear().apply()
    }

    private fun ensureKeyExists() {
        if (!keyStore.containsAlias(keyAlias)) {
            val keyGenerator = KeyGenerator.getInstance(
                KeyProperties.KEY_ALGORITHM_AES,
                "AndroidKeyStore"
            )
            val keyGenParameterSpec = KeyGenParameterSpec.Builder(
                keyAlias,
                KeyProperties.PURPOSE_ENCRYPT or KeyProperties.PURPOSE_DECRYPT
            )
                .setBlockModes(KeyProperties.BLOCK_MODE_GCM)
                .setEncryptionPaddings(KeyProperties.ENCRYPTION_PADDING_NONE)
                .setKeySize(256)
                .build()
            keyGenerator.init(keyGenParameterSpec)
            keyGenerator.generateKey()
        }
    }

    private fun getSecretKey(): SecretKey {
        return keyStore.getKey(keyAlias, null) as SecretKey
    }

    private fun encrypt(data: ByteArray): ByteArray {
        val cipher = Cipher.getInstance("AES/GCM/NoPadding")
        cipher.init(Cipher.ENCRYPT_MODE, getSecretKey())
        val iv = cipher.iv
        val encrypted = cipher.doFinal(data)
        
        // Prepend IV to encrypted data
        return iv + encrypted
    }

    private fun decrypt(encrypted: ByteArray): ByteArray {
        val cipher = Cipher.getInstance("AES/GCM/NoPadding")
        
        // Extract IV (first 12 bytes for GCM)
        val iv = encrypted.sliceArray(0 until 12)
        val encryptedData = encrypted.sliceArray(12 until encrypted.size)
        
        val spec = GCMParameterSpec(128, iv)
        cipher.init(Cipher.DECRYPT_MODE, getSecretKey(), spec)
        return cipher.doFinal(encryptedData)
    }
}

