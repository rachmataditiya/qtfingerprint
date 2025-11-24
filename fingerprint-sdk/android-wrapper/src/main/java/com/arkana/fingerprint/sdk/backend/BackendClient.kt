package com.arkana.fingerprint.sdk.backend

import com.arkana.fingerprint.sdk.model.Finger
import com.arkana.fingerprint.sdk.util.FingerError
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import okhttp3.*
import okhttp3.MediaType.Companion.toMediaType
import okhttp3.RequestBody.Companion.toRequestBody
import org.json.JSONException
import org.json.JSONObject
import java.util.Base64
import java.util.concurrent.TimeUnit

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
    private val client = OkHttpClient.Builder()
        .connectTimeout(10, java.util.concurrent.TimeUnit.SECONDS)
        .readTimeout(30, java.util.concurrent.TimeUnit.SECONDS)
        .writeTimeout(30, java.util.concurrent.TimeUnit.SECONDS)
        .retryOnConnectionFailure(true)
        .build()
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
                val errorBody = response.body?.string() ?: "Unknown error"
                android.util.Log.e("BackendClient", "storeTemplate failed: ${response.code} - $errorBody")
                BackendResult.Error(FingerError.BACKEND_ERROR, "Backend error: ${response.code} - $errorBody")
            }
        } catch (e: Exception) {
            android.util.Log.e("BackendClient", "storeTemplate exception", e)
            BackendResult.Error(FingerError.NETWORK_ERROR, "Network error: ${e.message}")
        }
    }

    /**
     * Load template from backend.
     * 
     * @param userId User ID
     * @param finger Optional finger type (e.g., "LEFT_INDEX"). If null, loads the most recent template.
     */
    suspend fun loadTemplate(userId: Int, finger: String? = null): BackendResult = withContext(Dispatchers.IO) {
        try {
            val url = if (finger != null) {
                "$baseUrl/users/$userId/fingerprint?finger=$finger"
            } else {
                "$baseUrl/users/$userId/fingerprint"
            }
            val request = Request.Builder()
                .url(url)
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
                val errorBody = response.body?.string() ?: "Unknown error"
                android.util.Log.e("BackendClient", "loadTemplate failed: ${response.code} - $errorBody")
                BackendResult.Error(FingerError.TEMPLATE_NOT_FOUND, "Template not found: ${response.code}")
            }
        } catch (e: Exception) {
            android.util.Log.e("BackendClient", "loadTemplate exception", e)
            BackendResult.Error(FingerError.NETWORK_ERROR, "Network error: ${e.message}")
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
                android.util.Log.d("BackendClient", "loadTemplates response body length: ${body?.length ?: 0}")
                val jsonArray = org.json.JSONArray(body)
                android.util.Log.d("BackendClient", "loadTemplates: Found ${jsonArray.length()} template(s) in response")
                
                // Load all templates (including multiple fingers per user)
                val templates = mutableListOf<TemplateInfo>()
                
                for (i in 0 until jsonArray.length()) {
                    val item = jsonArray.getJSONObject(i)
                    val userId = item.getInt("user_id")
                    // user_name should always be present from backend (JOIN with users table)
                    val userName = if (item.has("user_name") && !item.isNull("user_name")) {
                        item.getString("user_name")
                    } else {
                        android.util.Log.w("BackendClient", "loadTemplates: user_name is missing for user_id $userId, this should not happen")
                        "User $userId"
                    }
                    val userEmail = if (item.has("user_email") && !item.isNull("user_email")) {
                        item.getString("user_email")
                    } else {
                        null
                    }
                    val finger = item.getString("finger")
                    val templateBase64 = item.getString("template")
                    val template = Base64.getDecoder().decode(templateBase64)
                    
                    android.util.Log.d("BackendClient", "loadTemplates: User $userId ($userName), finger: $finger, template size: ${template.size} bytes")
                    
                    templates.add(TemplateInfo(userId, userName, userEmail, finger, template))
                }
                
                android.util.Log.d("BackendClient", "loadTemplates: Returning ${templates.size} template(s) (all fingers)")
                BackendResult.Templates(templates)
            } else {
                val errorBody = response.body?.string() ?: "Unknown error"
                android.util.Log.e("BackendClient", "loadTemplates failed: ${response.code} - $errorBody")
                BackendResult.Error(FingerError.BACKEND_ERROR, "Backend error: ${response.code}")
            }
        } catch (e: Exception) {
            android.util.Log.e("BackendClient", "loadTemplates exception", e)
            BackendResult.Error(FingerError.NETWORK_ERROR, "Network error: ${e.message}")
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
     * Create user.
     */
    suspend fun createUser(name: String, email: String?): BackendResult = withContext(Dispatchers.IO) {
        try {
            val json = JSONObject().apply {
                put("name", name)
                if (email != null) {
                    put("email", email)
                }
            }

            val request = Request.Builder()
                .url("$baseUrl/users")
                .post(json.toString().toRequestBody(jsonMediaType))
                .addHeader("Connection", "close")
                .build()

            val response = try {
                client.newCall(request).execute()
            } catch (e: java.net.SocketTimeoutException) {
                android.util.Log.e("BackendClient", "createUser: Connection timeout", e)
                return@withContext BackendResult.Error(FingerError.NETWORK_ERROR, "Connection timeout: ${e.message}")
            } catch (e: java.io.IOException) {
                android.util.Log.e("BackendClient", "createUser: Network error", e)
                return@withContext BackendResult.Error(FingerError.NETWORK_ERROR, "Network error: ${e.message}")
            } catch (e: Exception) {
                android.util.Log.e("BackendClient", "createUser: Unexpected error", e)
                return@withContext BackendResult.Error(FingerError.NETWORK_ERROR, "Unexpected error: ${e.message}")
            }
            
            val responseBody = try {
                response.body?.string() ?: ""
            } catch (e: java.io.EOFException) {
                android.util.Log.e("BackendClient", "createUser: Unexpected end of stream", e)
                return@withContext BackendResult.Error(FingerError.NETWORK_ERROR, "Connection closed unexpectedly. Please try again.")
            } catch (e: java.io.IOException) {
                android.util.Log.e("BackendClient", "createUser: Error reading response", e)
                return@withContext BackendResult.Error(FingerError.NETWORK_ERROR, "Error reading response: ${e.message}")
            }
            
            if (response.isSuccessful) {
                try {
                    if (responseBody.isEmpty()) {
                        android.util.Log.e("BackendClient", "createUser: Empty response body")
                        return@withContext BackendResult.Error(FingerError.BACKEND_ERROR, "Empty response from server")
                    }
                    val jsonResponse = JSONObject(responseBody)
                    val userId = jsonResponse.getInt("id")
                    return@withContext BackendResult.UserCreated(userId)
                } catch (e: JSONException) {
                    android.util.Log.e("BackendClient", "createUser: Failed to parse response: $responseBody", e)
                    return@withContext BackendResult.Error(FingerError.BACKEND_ERROR, "Invalid response format: ${e.message}")
                } catch (e: Exception) {
                    android.util.Log.e("BackendClient", "createUser: Failed to parse response", e)
                    return@withContext BackendResult.Error(FingerError.BACKEND_ERROR, "Failed to parse response: ${e.message}")
                }
            } else {
                // Try to parse error response
                val errorMessage = try {
                    if (responseBody.isNotEmpty()) {
                        val errorJson = JSONObject(responseBody)
                        errorJson.optString("error", "Unknown error")
                    } else {
                        "Unknown error (HTTP ${response.code})"
                    }
                } catch (e: Exception) {
                    "Backend error: HTTP ${response.code}"
                }
                
                android.util.Log.e("BackendClient", "createUser failed: ${response.code} - $errorMessage")
                
                // Map HTTP status codes to appropriate error messages
                val finalError = when (response.code) {
                    409 -> errorMessage // CONFLICT (duplicate email)
                    400 -> errorMessage // BAD_REQUEST (validation error)
                    else -> "Backend error: HTTP ${response.code} - $errorMessage"
                }
                
                BackendResult.Error(FingerError.BACKEND_ERROR, finalError)
            }
        } catch (e: Exception) {
            android.util.Log.e("BackendClient", "createUser exception", e)
            BackendResult.Error(FingerError.NETWORK_ERROR, "Network error: ${e.message}")
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
                val errorBody = response.body?.string() ?: "Unknown error"
                android.util.Log.e("BackendClient", "listUsers failed: ${response.code} - $errorBody")
                BackendResult.Error(FingerError.BACKEND_ERROR, "Backend error: ${response.code}")
            }
        } catch (e: Exception) {
            android.util.Log.e("BackendClient", "listUsers exception", e)
            BackendResult.Error(FingerError.NETWORK_ERROR, "Network error: ${e.message}")
        }
    }

    /**
     * Get user's fingers.
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
                val errorBody = response.body?.string() ?: "Unknown error"
                android.util.Log.e("BackendClient", "getUserFingers failed: ${response.code} - $errorBody")
                BackendResult.Error(FingerError.BACKEND_ERROR, "Backend error: ${response.code}")
            }
        } catch (e: Exception) {
            android.util.Log.e("BackendClient", "getUserFingers exception", e)
            BackendResult.Error(FingerError.NETWORK_ERROR, "Network error: ${e.message}")
        }
    }
}

/**
 * User information model.
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
/**
 * Template info for identification (includes all fingers per user)
 */
data class TemplateInfo(
    val userId: Int,
    val userName: String,
    val userEmail: String?,
    val finger: String,
    val template: ByteArray
) {
    override fun equals(other: Any?): Boolean {
        if (this === other) return true
        if (javaClass != other?.javaClass) return false

        other as TemplateInfo

        if (userId != other.userId) return false
        if (userName != other.userName) return false
        if (userEmail != other.userEmail) return false
        if (finger != other.finger) return false
        if (!template.contentEquals(other.template)) return false

        return true
    }

    override fun hashCode(): Int {
        var result = userId
        result = 31 * result + userName.hashCode()
        result = 31 * result + (userEmail?.hashCode() ?: 0)
        result = 31 * result + finger.hashCode()
        result = 31 * result + template.contentHashCode()
        return result
    }
}

sealed class BackendResult {
    object Success : BackendResult()
    data class Template(val template: ByteArray) : BackendResult()
    data class Templates(val templates: List<TemplateInfo>) : BackendResult()
    data class UserCreated(val userId: Int) : BackendResult()
    data class Users(val users: List<UserInfo>) : BackendResult()
    data class Fingers(val fingers: List<String>) : BackendResult()
    data class Error(val error: FingerError, val message: String? = null) : BackendResult()
}
