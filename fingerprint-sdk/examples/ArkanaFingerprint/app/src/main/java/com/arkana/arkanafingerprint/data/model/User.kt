package com.arkana.arkanafingerprint.data.model

data class User(
    val id: Int,
    val name: String,
    val email: String?,
    val fingerCount: Int
)

