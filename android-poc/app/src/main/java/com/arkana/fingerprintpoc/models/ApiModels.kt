package com.arkana.fingerprintpoc.models

import com.google.gson.annotations.SerializedName

data class HealthResponse(
    val status: String,
    val service: String
)

data class UserResponse(
    val id: Int,
    val name: String,
    val email: String?,
    @SerializedName("has_fingerprint")
    val hasFingerprint: Boolean,
    @SerializedName("created_at")
    val createdAt: String,
    @SerializedName("updated_at")
    val updatedAt: String
)

data class CreateUserRequest(
    val name: String,
    val email: String?
)

data class EnrollFingerprintRequest(
    val template: String // Base64 encoded
)

data class VerifyFingerprintRequest(
    val template: String // Base64 encoded
)

data class IdentifyFingerprintRequest(
    val template: String // Base64 encoded
)

data class VerifyResponse(
    val matched: Boolean,
    val score: Int?
)

data class IdentifyResponse(
    @SerializedName("user_id")
    val userId: Int?,
    val score: Int?
)

data class FingerprintResponse(
    val template: String // Base64 encoded
)

