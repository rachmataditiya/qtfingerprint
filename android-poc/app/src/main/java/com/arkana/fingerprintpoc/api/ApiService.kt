package com.arkana.fingerprintpoc.api

import com.arkana.fingerprintpoc.models.*
import retrofit2.Response
import retrofit2.http.*

interface ApiService {
    
    @GET("health")
    suspend fun healthCheck(): Response<HealthResponse>
    
    @GET("users")
    suspend fun listUsers(): Response<List<UserResponse>>
    
    @GET("users/{id}")
    suspend fun getUser(@Path("id") id: Int): Response<UserResponse>
    
    @POST("users")
    suspend fun createUser(@Body request: CreateUserRequest): Response<UserResponse>
    
    @DELETE("users/{id}")
    suspend fun deleteUser(@Path("id") id: Int): Response<Unit>
    
    @POST("users/{id}/fingerprint")
    suspend fun enrollFingerprint(
        @Path("id") userId: Int,
        @Body request: EnrollFingerprintRequest
    ): Response<UserResponse>
    
    @GET("users/{id}/fingerprint")
    suspend fun getFingerprint(@Path("id") id: Int): Response<FingerprintResponse>
    
    @POST("users/{id}/verify")
    suspend fun verifyFingerprint(
        @Path("id") userId: Int,
        @Body request: VerifyFingerprintRequest
    ): Response<VerifyResponse>
    
    @POST("identify")
    suspend fun identifyUser(@Body request: IdentifyFingerprintRequest): Response<IdentifyResponse>
}

