package com.arkana.fingerprint.sdk.examples

import android.os.Bundle
import androidx.appcompat.app.AppCompatActivity
import com.arkana.fingerprint.sdk.FingerprintSdk
import com.arkana.fingerprint.sdk.config.FingerprintSdkConfig
import com.arkana.fingerprint.sdk.model.*
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch

/**
 * Basic usage example of Arkana Fingerprint SDK.
 */
class BasicUsageActivity : AppCompatActivity() {
    private val scope = CoroutineScope(Dispatchers.Main)

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        // Initialize SDK
        val config = FingerprintSdkConfig(
            backendUrl = "http://api.example.com",
            enrollmentScans = 5,
            matchThreshold = 0.6f,
            enableCache = true
        )
        FingerprintSdk.init(config, this)

        // Example: Enroll fingerprint
        enrollExample()

        // Example: Verify fingerprint
        verifyExample()

        // Example: Identify user
        identifyExample()
    }

    private fun enrollExample() {
        scope.launch {
            val result = FingerprintSdk.enroll(
                userId = 123,
                finger = Finger.LEFT_INDEX
            )

            when (result) {
                is EnrollResult.Success -> {
                    println("✅ Enrollment successful for user ${result.userId}")
                }
                is EnrollResult.Error -> {
                    println("❌ Enrollment failed: ${result.error}")
                }
            }
        }
    }

    private fun verifyExample() {
        scope.launch {
            val result = FingerprintSdk.verify(userId = 123)

            when (result) {
                is VerifyResult.Success -> {
                    println("✅ Verified! User: ${result.userId}, Score: ${result.score}")
                }
                is VerifyResult.Error -> {
                    println("❌ Verification failed: ${result.error}")
                }
            }
        }
    }

    private fun identifyExample() {
        scope.launch {
            val result = FingerprintSdk.identify(scope = "branch_001")

            when (result) {
                is IdentifyResult.Success -> {
                    println("✅ User identified: ${result.userId}, Score: ${result.score}")
                }
                is IdentifyResult.Error -> {
                    println("❌ Identification failed: ${result.error}")
                }
            }
        }
    }
}

