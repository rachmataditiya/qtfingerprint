package com.arkana.fingerprintpoc

import android.os.Bundle
import android.util.Base64
import android.util.Log
import android.view.View
import android.widget.Toast
import androidx.appcompat.app.AlertDialog
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.lifecycleScope
import com.arkana.fingerprintpoc.api.ApiClient
import com.arkana.fingerprintpoc.databinding.ActivityMainBinding
import com.arkana.fingerprintpoc.models.UserResponse
import kotlinx.coroutines.launch

class MainActivity : AppCompatActivity() {
    
    private lateinit var binding: ActivityMainBinding
    private lateinit var fingerprintManager: FingerprintManager
    
    private var isInitialized = false
    private var selectedUserId: Int? = null
    private var selectedUser: UserResponse? = null
    
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)
        
        fingerprintManager = FingerprintManager(this)
        
        setupUI()
    }
    
    private fun setupUI() {
        // Initialize USB Device button
        binding.btnInitialize.setOnClickListener {
            initializeReader()
        }
        
        // Verification button
        binding.btnStartVerify.setOnClickListener {
            if (!isInitialized) {
                Toast.makeText(this, "Please initialize USB device first", Toast.LENGTH_SHORT).show()
                return@setOnClickListener
            }
            
            if (selectedUserId == null) {
                showUserSelectionDialog { userId ->
                    selectedUserId = userId
                    loadUserInfo(userId)
                    startVerification(userId)
                }
            } else {
                startVerification(selectedUserId!!)
            }
        }
        
        // Identification button
        binding.btnIdentify.setOnClickListener {
            if (!isInitialized) {
                Toast.makeText(this, "Please initialize USB device first", Toast.LENGTH_SHORT).show()
                return@setOnClickListener
            }
            startIdentification()
        }
        
        // Initial state
        updateStatus("Ready - Initialize USB device to begin", false)
        updateUserInfo(null)
        enableControls(false)
        hideResult()
    }
    
    private fun initializeReader() {
        binding.btnInitialize.isEnabled = false
        binding.btnInitialize.text = "Initializing..."
        updateStatus("Initializing USB fingerprint device...", false)
        
        val available = fingerprintManager.initialize(this)
        
        if (available) {
            isInitialized = true
            updateStatus("USB device initialized successfully", false)
            enableControls(true)
            binding.btnInitialize.text = "Device Ready"
        } else {
            updateStatus("USB device not available - Please connect fingerprint reader", true)
            enableControls(false)
            binding.btnInitialize.text = "Initialize USB Device"
        }
        
        binding.btnInitialize.isEnabled = true
    }
    
    private fun showUserSelectionDialog(onUserSelected: (Int) -> Unit) {
        lifecycleScope.launch {
            try {
                val response = ApiClient.apiService.listUsers()
                if (response.isSuccessful && response.body() != null) {
                    val users = response.body()!!.filter { it.hasFingerprint }
                    
                    if (users.isEmpty()) {
                        runOnUiThread {
                            Toast.makeText(this@MainActivity, "No users with fingerprints found", Toast.LENGTH_SHORT).show()
                        }
                        return@launch
                    }
                    
                    val userNames = users.map { "${it.name} (ID: ${it.id})" }.toTypedArray()
                    
                    runOnUiThread {
                        AlertDialog.Builder(this@MainActivity)
                            .setTitle("Select User for Verification")
                            .setItems(userNames) { _, which ->
                                onUserSelected(users[which].id)
                            }
                            .setNegativeButton("Cancel", null)
                            .show()
                    }
                } else {
                    runOnUiThread {
                        Toast.makeText(this@MainActivity, "Failed to load users", Toast.LENGTH_SHORT).show()
                    }
                }
            } catch (e: Exception) {
                Log.e("MainActivity", "Error loading users: ${e.message}", e)
                runOnUiThread {
                    Toast.makeText(this@MainActivity, "Error: ${e.message}", Toast.LENGTH_SHORT).show()
                }
            }
        }
    }
    
    private fun loadUserInfo(userId: Int) {
        lifecycleScope.launch {
            try {
                val response = ApiClient.apiService.getUser(userId)
                if (response.isSuccessful && response.body() != null) {
                    val user = response.body()!!
                    selectedUser = user
                    runOnUiThread {
                        updateUserInfo(user)
                    }
                }
            } catch (e: Exception) {
                Log.e("MainActivity", "Error loading user info: ${e.message}", e)
            }
        }
    }
    
    private fun startVerification(userId: Int) {
        binding.btnStartVerify.isEnabled = false
        hideResult()
        updateStatus("Loading template from database...", false)
        
        lifecycleScope.launch {
            try {
                // Step 1: Get stored fingerprint template from API (Base64 encoded)
                updateStatus("Fetching stored template for user $userId...", false)
                val fingerprintResponse = ApiClient.apiService.getFingerprint(userId)

                if (!fingerprintResponse.isSuccessful || fingerprintResponse.body() == null) {
                    runOnUiThread {
                        binding.btnStartVerify.isEnabled = true
                        showResult("ERROR", "Failed to fetch stored template", false)
                        updateStatus("Failed to fetch stored template: ${fingerprintResponse.message()}", true)
                    }
                    return@launch
                }

                val storedTemplateBase64 = fingerprintResponse.body()!!.template
                val storedTemplate = Base64.decode(storedTemplateBase64, Base64.DEFAULT)

                if (storedTemplate.isEmpty()) {
                    runOnUiThread {
                        binding.btnStartVerify.isEnabled = true
                        showResult("NO TEMPLATE", "User has no fingerprint template", false)
                        updateStatus("Stored template is empty", true)
                    }
                    return@launch
                }
                Log.d("MainActivity", "Fetched stored template: ${storedTemplate.size} bytes")

                // Step 2: Capture live fingerprint using libfprint and perform local matching
                updateStatus("Place your finger on the reader...", false)
                val (matched, score) = fingerprintManager.verifyFingerprint(storedTemplate)

                runOnUiThread {
                    binding.btnStartVerify.isEnabled = true

                    if (matched) {
                        showResult("VERIFIED", "Identity verified successfully\nScore: $score%", true)
                        updateStatus("Verification successful! User ID: $userId", false)
                    } else {
                        showResult("NOT VERIFIED", "Fingerprint does not match", false)
                        updateStatus("Verification failed - Fingerprint does not match", true)
                    }
                }
            } catch (e: Exception) {
                Log.e("MainActivity", "Verification error: ${e.message}", e)
                runOnUiThread {
                    binding.btnStartVerify.isEnabled = true
                    showResult("ERROR", "Error: ${e.message}", false)
                    updateStatus("Error: ${e.message}", true)
                }
            }
        }
    }
    
    private fun startIdentification() {
        binding.btnIdentify.isEnabled = false
        hideResult()
        updateStatus("Loading users from database...", false)

        lifecycleScope.launch {
            try {
                // Step 1: Get all user templates from API
                updateStatus("Fetching all user templates for identification...", false)
                val allUsersResponse = ApiClient.apiService.listUsers()

                if (!allUsersResponse.isSuccessful || allUsersResponse.body() == null) {
                    runOnUiThread {
                        binding.btnIdentify.isEnabled = true
                        showResult("ERROR", "Failed to fetch user list", false)
                        updateStatus("Failed to fetch user list: ${allUsersResponse.message()}", true)
                    }
                    return@launch
                }

                val usersWithFingerprints = allUsersResponse.body()!!.filter { it.hasFingerprint }
                if (usersWithFingerprints.isEmpty()) {
                    runOnUiThread {
                        binding.btnIdentify.isEnabled = true
                        showResult("NO USERS", "No users with fingerprints found", false)
                        updateStatus("No users with fingerprints found for identification.", true)
                    }
                    return@launch
                }

                val userTemplatesMap = mutableMapOf<Int, ByteArray>()
                for (user in usersWithFingerprints) {
                    val fpResponse = ApiClient.apiService.getFingerprint(user.id)
                    if (fpResponse.isSuccessful && fpResponse.body() != null) {
                        val templateBase64 = fpResponse.body()!!.template
                        val template = Base64.decode(templateBase64, Base64.DEFAULT)
                        if (template.isNotEmpty()) {
                            userTemplatesMap[user.id] = template
                        }
                    }
                }

                if (userTemplatesMap.isEmpty()) {
                    runOnUiThread {
                        binding.btnIdentify.isEnabled = true
                        showResult("NO TEMPLATES", "No valid fingerprint templates found", false)
                        updateStatus("No valid fingerprint templates found for identification.", true)
                    }
                    return@launch
                }
                Log.d("MainActivity", "Fetched ${userTemplatesMap.size} templates for identification")

                // Step 2: Capture live fingerprint and perform local identification
                updateStatus("Place your finger on the reader...", false)
                val (matchedUserId, score) = fingerprintManager.identifyUser(userTemplatesMap)

                runOnUiThread {
                    binding.btnIdentify.isEnabled = true

                    if (matchedUserId != -1 && score >= 60) {
                        // Load user info for display
                        selectedUserId = matchedUserId
                        loadUserInfo(matchedUserId)
                        
                        showResult("IDENTIFIED", "User ID: $matchedUserId\nScore: $score%", true)
                        updateStatus("User identified! ID: $matchedUserId, Score: $score%", false)
                        Toast.makeText(this@MainActivity, "User identified: ID $matchedUserId (Score: $score%)", Toast.LENGTH_LONG).show()
                    } else {
                        showResult("NOT FOUND", "No matching user found", false)
                        updateStatus("No matching user found.", true)
                        Toast.makeText(this@MainActivity, "No matching user found", Toast.LENGTH_SHORT).show()
                    }
                }
            } catch (e: Exception) {
                Log.e("MainActivity", "Identification error: ${e.message}", e)
                runOnUiThread {
                    binding.btnIdentify.isEnabled = true
                    showResult("ERROR", "Error: ${e.message}", false)
                    updateStatus("Error: ${e.message}", true)
                }
            }
        }
    }
    
    private fun updateUserInfo(user: UserResponse?) {
        if (user == null) {
            binding.userInfoLabel.text = "No user selected"
            binding.userEmailLabel.visibility = View.GONE
            selectedUserId = null
        } else {
            binding.userInfoLabel.text = user.name
            selectedUserId = user.id
            if (!user.email.isNullOrEmpty()) {
                binding.userEmailLabel.text = user.email
                binding.userEmailLabel.visibility = View.VISIBLE
            } else {
                binding.userEmailLabel.visibility = View.GONE
            }
        }
    }
    
    private fun enableControls(enabled: Boolean) {
        // Verification button is enabled if device is initialized (user can be selected via dialog)
        binding.btnStartVerify.isEnabled = enabled
        binding.btnIdentify.isEnabled = enabled
    }
    
    private fun updateStatus(message: String, isError: Boolean) {
        binding.statusLabel.text = message
        binding.statusLabel.setBackgroundColor(
            if (isError) getColor(android.R.color.holo_red_light)
            else getColor(android.R.color.holo_green_light)
        )
        Log.d("MainActivity", "Status: $message")
    }
    
    private fun showResult(title: String, message: String, isSuccess: Boolean) {
        binding.resultCard.visibility = View.VISIBLE
        binding.resultLabel.text = "$title\n$message"
        
        val iconRes = if (isSuccess) android.R.drawable.ic_dialog_info else android.R.drawable.ic_dialog_alert
        binding.resultIcon.setImageResource(iconRes)
        binding.resultIcon.setColorFilter(
            if (isSuccess) getColor(android.R.color.holo_green_dark)
            else getColor(android.R.color.holo_red_dark)
        )
        
        binding.resultCard.setCardBackgroundColor(
            if (isSuccess) getColor(android.R.color.holo_green_light)
            else getColor(android.R.color.holo_red_light)
        )
    }
    
    private fun hideResult() {
        binding.resultCard.visibility = View.GONE
    }
    
    override fun onDestroy() {
        super.onDestroy()
        fingerprintManager.cleanup()
    }
}
