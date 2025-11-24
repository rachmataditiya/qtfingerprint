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
                BackendResult.Template(template)
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
                
                BackendResult.Templates(templates)
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

    /**
     * List all users.
     */
    suspend fun listUsers(): BackendResult = withContext(Dispatchers.IO) {
        try {
            val request = Request.Builder()
                .url("$baseUrl/users")
                .get()
                .build()

            val response = client.newCall(request).execute()
            if (response.isSuccessful) {
                val body = response.body?.string()
                val jsonArray = org.json.JSONArray(body)
                val users = mutableListOf<UserInfo>()
                
                for (i in 0 until jsonArray.length()) {
                    val item = jsonArray.getJSONObject(i)
                    users.add(UserInfo(
                        id = item.getInt("id"),
                        name = item.getString("name"),
                        email = if (item.has("email") && !item.isNull("email")) item.getString("email") else null,
                        fingerCount = item.getInt("finger_count")
                    ))
                }
                
                BackendResult.Users(users)
            } else {
                BackendResult.Error(FingerError.BACKEND_ERROR)
            }
        } catch (e: Exception) {
            BackendResult.Error(FingerError.NETWORK_ERROR)
        }
    }

    /**
     * Create a new user.
     */
    suspend fun createUser(name: String, email: String?): BackendResult = withContext(Dispatchers.IO) {
        try {
            val json = JSONObject().apply {
                put("name", name)
                if (email != null && email.isNotBlank()) {
                    put("email", email)
                }
            }

            val request = Request.Builder()
                .url("$baseUrl/users")
                .post(json.toString().toRequestBody(jsonMediaType))
                .build()

            val response = client.newCall(request).execute()
            if (response.isSuccessful) {
                val body = response.body?.string()
                val json = JSONObject(body)
                val user = UserInfo(
                    id = json.getInt("id"),
                    name = json.getString("name"),
                    email = if (json.has("email") && !json.isNull("email")) json.getString("email") else null,
                    fingerCount = json.getInt("finger_count")
                )
                BackendResult.UserCreated(user)
            } else {
                val errorBody = response.body?.string()
                val errorMessage = try {
                    val errorJson = JSONObject(errorBody)
                    errorJson.getString("error")
                } catch (e: Exception) {
                    "Failed to create user"
                }
                BackendResult.Error(FingerError.BACKEND_ERROR, errorMessage)
            }
        } catch (e: Exception) {
            BackendResult.Error(FingerError.NETWORK_ERROR, e.message ?: "Network error")
        }
    }

    /**
     * Get user fingers.
     */
    suspend fun getUserFingers(userId: Int): BackendResult = withContext(Dispatchers.IO) {
        try {
            val request = Request.Builder()
                .url("$baseUrl/users/$userId/fingers")
                .get()
                .build()

            val response = client.newCall(request).execute()
            if (response.isSuccessful) {
                val body = response.body?.string()
                val jsonArray = org.json.JSONArray(body)
                val fingers = mutableListOf<String>()
                
                for (i in 0 until jsonArray.length()) {
                    fingers.add(jsonArray.getString(i))
                }
                
                BackendResult.Fingers(fingers)
            } else {
                BackendResult.Error(FingerError.BACKEND_ERROR)
            }
        } catch (e: Exception) {
            BackendResult.Error(FingerError.NETWORK_ERROR)
        }
    }
}

/**
 * User information from backend.
 */
data class UserInfo(
    val id: Int,
    val name: String,
    val email: String?,
    val fingerCount: Int
)

/**
 * Backend operation result.
 */
sealed class BackendResult {
    object Success : BackendResult()
    data class Template(val template: ByteArray) : BackendResult()
    data class Templates(val templates: Map<Int, ByteArray>) : BackendResult()
    data class Users(val users: List<UserInfo>) : BackendResult()
    data class UserCreated(val user: UserInfo) : BackendResult()
    data class Fingers(val fingers: List<String>) : BackendResult()
    data class Error(val error: FingerError, val message: String? = null) : BackendResult()
}
