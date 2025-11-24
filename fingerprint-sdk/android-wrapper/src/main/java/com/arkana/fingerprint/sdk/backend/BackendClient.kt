package com.arkana.fingerprint.sdk.backend

import com.arkana.fingerprint.sdk.model.Finger
import com.arkana.fingerprint.sdk.util.FingerError
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import okhttp3.*
import okhttp3.MediaType.Companion.toMediaType
import okhttp3.RequestBody.Companion.toRequestBody
import org.json.JSONObject
import java.util.Base64

/**
 * Backend API client for template storage and retrieval.
 * 
 * Communicates with Rust Axum API:
 * - POST /enroll
 * - GET /template/{user_id}
 * - GET /templates?scope=branch
 * - POST /log_auth
 */
class BackendClient(private val baseUrl: String) {
    private val client = OkHttpClient()
    private val jsonMediaType = "application/json".toMediaType()

    /**
     * Store template in backend.
     */
    suspend fun storeTemplate(
        userId: Int,
        template: ByteArray,
        finger: Finger
    ): BackendResult = withContext(Dispatchers.IO) {
        try {
            val templateBase64 = Base64.getEncoder().encodeToString(template)
            val json = JSONObject().apply {
                put("template", templateBase64)
                put("finger", finger.name)
            }

            val request = Request.Builder()
                .url("$baseUrl/users/$userId/fingerprint")
                .post(json.toString().toRequestBody(jsonMediaType))
                .build()

            val response = client.newCall(request).execute()
            if (response.isSuccessful) {
                BackendResult.Success
            } else {
                BackendResult.Error(FingerError.BACKEND_ERROR)
            }
        } catch (e: Exception) {
            BackendResult.Error(FingerError.NETWORK_ERROR)
        }
    }

    /**
     * Load template from backend.
     */
    suspend fun loadTemplate(userId: Int): BackendResult = withContext(Dispatchers.IO) {
        try {
            val request = Request.Builder()
                .url("$baseUrl/users/$userId/fingerprint")
                .get()
                .build()

            val response = client.newCall(request).execute()
            if (response.isSuccessful) {
                val body = response.body?.string()
                val json = JSONObject(body)
                val templateBase64 = json.getString("template")
                val template = Base64.getDecoder().decode(templateBase64)
                BackendResult.Success(template)
            } else {
                BackendResult.Error(FingerError.TEMPLATE_NOT_FOUND)
            }
        } catch (e: Exception) {
            BackendResult.Error(FingerError.NETWORK_ERROR)
        }
    }

    /**
     * Load all templates from backend.
     */
    suspend fun loadTemplates(scope: String?): BackendResult = withContext(Dispatchers.IO) {
        try {
            val url = if (scope != null) {
                "$baseUrl/templates?scope=$scope"
            } else {
                "$baseUrl/templates"
            }

            val request = Request.Builder()
                .url(url)
                .get()
                .build()

            val response = client.newCall(request).execute()
            if (response.isSuccessful) {
                val body = response.body?.string()
                val jsonArray = org.json.JSONArray(body)
                val templates = mutableMapOf<Int, ByteArray>()
                
                for (i in 0 until jsonArray.length()) {
                    val item = jsonArray.getJSONObject(i)
                    val userId = item.getInt("user_id")
                    val templateBase64 = item.getString("template")
                    val template = Base64.getDecoder().decode(templateBase64)
                    templates[userId] = template
                }
                
                BackendResult.Success(templates)
            } else {
                BackendResult.Error(FingerError.BACKEND_ERROR)
            }
        } catch (e: Exception) {
            BackendResult.Error(FingerError.NETWORK_ERROR)
        }
    }

    /**
     * Log authentication event.
     */
    suspend fun logAuth(userId: Int, success: Boolean, score: Float) {
        withContext(Dispatchers.IO) {
            try {
                val json = JSONObject().apply {
                    put("user_id", userId)
                    put("success", success)
                    put("score", score)
                }

                val request = Request.Builder()
                    .url("$baseUrl/log_auth")
                    .post(json.toString().toRequestBody(jsonMediaType))
                    .build()

                client.newCall(request).execute() // Fire and forget
            } catch (e: Exception) {
                // Ignore errors
            }
        }
    }
}

/**
 * Backend operation result.
 */
sealed class BackendResult {
    object Success : BackendResult()
    data class Success(val template: ByteArray) : BackendResult()
    data class Success(val templates: Map<Int, ByteArray>) : BackendResult()
    data class Error(val error: FingerError) : BackendResult()
}

