package com.arkana.fingerprintpoc

import android.os.Bundle
import android.util.Log
import android.view.View
import android.widget.Toast
import androidx.appcompat.app.AlertDialog
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.lifecycleScope
import androidx.recyclerview.widget.LinearLayoutManager
import com.arkana.fingerprintpoc.adapter.UserListAdapter
import com.arkana.fingerprintpoc.api.ApiClient
import com.arkana.fingerprintpoc.databinding.ActivityMainBinding
import com.arkana.fingerprintpoc.models.*
import kotlinx.coroutines.launch
import java.io.IOException

class MainActivity : AppCompatActivity() {
    
    private lateinit var binding: ActivityMainBinding
    private lateinit var fingerprintManager: FingerprintManager
    private lateinit var userAdapter: UserListAdapter
    
    private var isInitialized = false
    var selectedUserId: Int? = null
        private set
    
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)
        
        fingerprintManager = FingerprintManager(this)
        
        setupUI()
        setupRecyclerView()
    }
    
    private fun setupUI() {
        // Initialize Reader button
        binding.btnInitialize.setOnClickListener {
            initializeReader()
        }
        
        // Verification - Use selected user from list
        binding.btnStartVerify.setOnClickListener {
            val userId = selectedUserId
            if (userId == null || userId <= 0) {
                Toast.makeText(this, "Please select a user from the list first", Toast.LENGTH_SHORT).show()
                return@setOnClickListener
            }
            
            if (!isInitialized) {
                Toast.makeText(this, "Please initialize fingerprint reader first", Toast.LENGTH_SHORT).show()
                return@setOnClickListener
            }
            
            startVerification(userId)
        }
        
        // Identification
        binding.btnIdentify.setOnClickListener {
            startIdentification()
        }
        
        // Refresh user list
        binding.btnRefreshList.setOnClickListener {
            loadUsers()
        }
        
        // Delete user
        binding.btnDeleteUser.setOnClickListener {
            deleteSelectedUser()
        }
        
        // Initial state
        updateStatus("Ready - Tap 'Initialize Reader' to begin", false)
        enableControls(false)
    }
    
    private fun setupRecyclerView() {
        userAdapter = UserListAdapter { user ->
            binding.selectedUserName.text = "Selected: ${user.name}"
            selectedUserId = user.id
            binding.btnDeleteUser.isEnabled = true
        }
        
        binding.recyclerViewUsers.apply {
            layoutManager = LinearLayoutManager(this@MainActivity)
            adapter = userAdapter
        }
    }
    
    private fun initializeReader() {
        binding.btnInitialize.isEnabled = false
        binding.btnInitialize.text = "Initializing..."
        
        val available = fingerprintManager.initialize(this)
        
        if (available) {
            isInitialized = true
            updateStatus("libfprint initialized successfully", false)
            enableControls(true)
            loadUsers() // Load users from API server
        } else {
            updateStatus("libfprint not available - Please connect fingerprint reader", true)
            enableControls(false)
        }
        
        binding.btnInitialize.isEnabled = true
        binding.btnInitialize.text = "Initialize Reader"
    }
    
    private fun startVerification(userId: Int) {
        binding.btnStartVerify.isEnabled = false
        updateStatus("Loading template from database...", false)
        
        lifecycleScope.launch {
            try {
                // Step 1: Load fingerprint template from database (middleware is just DB bridge)
                updateStatus("Loading fingerprint template from database...", false)
                val fingerprintResponse = ApiClient.apiService.getFingerprint(userId)
                
                if (!fingerprintResponse.isSuccessful || fingerprintResponse.body() == null) {
                    runOnUiThread {
                        binding.btnStartVerify.isEnabled = true
                        binding.verifyResultLabel.visibility = View.VISIBLE
                        binding.verifyResultLabel.text = "NO TEMPLATE"
                        binding.verifyResultLabel.setBackgroundColor(getColor(android.R.color.holo_orange_light))
                        updateStatus("User has no fingerprint template. Please enroll first.", true)
                    }
                    return@launch
                }
                
                val storedTemplateHex = fingerprintResponse.body()!!.template
                val storedTemplate = hexStringToByteArray(storedTemplateHex)
                
                // Step 2: Capture fingerprint image using libfprint
                updateStatus("Capturing fingerprint using libfprint...", false)
                val imageData = fingerprintManager.captureFingerprintTemplate()
                
                if (imageData == null || imageData.isEmpty()) {
                    runOnUiThread {
                        binding.btnStartVerify.isEnabled = true
                        binding.verifyResultLabel.visibility = View.VISIBLE
                        binding.verifyResultLabel.text = "CAPTURE FAILED"
                        binding.verifyResultLabel.setBackgroundColor(getColor(android.R.color.holo_red_light))
                        updateStatus("Failed to capture fingerprint", true)
                    }
                    return@launch
                }
                
                // Step 3: Match locally using libfprint (like Qt does)
                updateStatus("Matching locally using libfprint...", false)
                val (matched, score) = fingerprintManager.verifyFingerprint(storedTemplate)
                
                runOnUiThread {
                    binding.btnStartVerify.isEnabled = true
                    binding.verifyResultLabel.visibility = View.VISIBLE
                    
                    if (matched && score >= 60) {
                        binding.verifyResultLabel.text = "✓ MATCH - Score: $score%"
                        binding.verifyResultLabel.setBackgroundColor(getColor(android.R.color.holo_green_light))
                        updateStatus("Verification successful! User ID: $userId", false)
                    } else {
                        binding.verifyResultLabel.text = "✗ NO MATCH"
                        binding.verifyResultLabel.setBackgroundColor(getColor(android.R.color.holo_red_light))
                        updateStatus("Verification failed - Fingerprint does not match", true)
                    }
                }
            } catch (e: Exception) {
                Log.e("MainActivity", "Verification error: ${e.message}", e)
                runOnUiThread {
                    binding.btnStartVerify.isEnabled = true
                    binding.verifyResultLabel.visibility = View.VISIBLE
                    binding.verifyResultLabel.text = "ERROR: ${e.message}"
                    binding.verifyResultLabel.setBackgroundColor(getColor(android.R.color.holo_red_light))
                    updateStatus("Error: ${e.message}", true)
                }
            }
        }
    }
    
    private fun hexStringToByteArray(hex: String): ByteArray {
        val len = hex.length
        val data = ByteArray(len / 2)
        var i = 0
        while (i < len) {
            data[i / 2] = ((Character.digit(hex[i], 16) shl 4) + Character.digit(hex[i + 1], 16)).toByte()
            i += 2
        }
        return data
    }
    
    private fun startIdentification() {
        if (!isInitialized) {
            Toast.makeText(this, "Please initialize fingerprint reader first", Toast.LENGTH_SHORT).show()
            return
        }
        
        binding.btnIdentify.isEnabled = false
        updateStatus("Loading users from database...", false)
        
        lifecycleScope.launch {
            try {
                // Step 1: Load all users with fingerprints from database (middleware is just DB bridge)
                updateStatus("Loading users with fingerprints from database...", false)
                val usersResponse = ApiClient.apiService.listUsers()
                
                if (!usersResponse.isSuccessful || usersResponse.body() == null) {
                    runOnUiThread {
                        binding.btnIdentify.isEnabled = true
                        updateStatus("Failed to load users", true)
                        Toast.makeText(this@MainActivity, "Failed to load users", Toast.LENGTH_SHORT).show()
                    }
                    return@launch
                }
                
                val users = usersResponse.body()!!.filter { it.hasFingerprint }
                
                if (users.isEmpty()) {
                    runOnUiThread {
                        binding.btnIdentify.isEnabled = true
                        updateStatus("No users with fingerprints found", true)
                        Toast.makeText(this@MainActivity, "No users with fingerprints found", Toast.LENGTH_SHORT).show()
                    }
                    return@launch
                }
                
                // Step 2: Capture fingerprint image using libfprint
                updateStatus("Capturing fingerprint using libfprint...", false)
                val imageData = fingerprintManager.captureFingerprintTemplate()
                
                if (imageData == null || imageData.isEmpty()) {
                    runOnUiThread {
                        binding.btnIdentify.isEnabled = true
                        updateStatus("Failed to capture fingerprint. Please try again.", true)
                        Toast.makeText(this@MainActivity, "Failed to capture fingerprint", Toast.LENGTH_SHORT).show()
                    }
                    return@launch
                }
                
                Log.d("MainActivity", "Captured fingerprint image: ${imageData.size} bytes for identification")
                
                // Step 3: Load all templates and match locally using libfprint (like Qt does)
                updateStatus("Loading templates and matching locally...", false)
                
                // Load all templates from database
                val userTemplates = mutableMapOf<Int, ByteArray>()
                for (user in users) {
                    try {
                        val fingerprintResponse = ApiClient.apiService.getFingerprint(user.id)
                        if (fingerprintResponse.isSuccessful && fingerprintResponse.body() != null) {
                            val templateHex = fingerprintResponse.body()!!.template
                            val templateData = hexStringToByteArray(templateHex)
                            userTemplates[user.id] = templateData
                        }
                    } catch (e: Exception) {
                        Log.e("MainActivity", "Failed to load template for user ${user.id}: ${e.message}")
                    }
                }
                
                if (userTemplates.isEmpty()) {
                    runOnUiThread {
                        binding.btnIdentify.isEnabled = true
                        updateStatus("No templates found", true)
                        Toast.makeText(this@MainActivity, "No templates found", Toast.LENGTH_SHORT).show()
                    }
                    return@launch
                }
                
                updateStatus("Matching locally against ${userTemplates.size} users...", false)
                val (matchedUserId, score) = fingerprintManager.identifyUser(userTemplates)
                
                runOnUiThread {
                    binding.btnIdentify.isEnabled = true
                    
                    if (matchedUserId >= 0 && score >= 60) {
                        updateStatus("User identified! ID: $matchedUserId, Score: $score%", false)
                        
                        // Show success message
                        Toast.makeText(this@MainActivity, "User identified: ID $matchedUserId (Score: $score%)", Toast.LENGTH_LONG).show()
                        
                        // Select and highlight the identified user in the list
                        selectedUserId = matchedUserId
                        
                        // Refresh user list to show the identified user
                        loadUsers()
                        
                        // Update selected user display
                        lifecycleScope.launch {
                            try {
                                val userResponse = ApiClient.apiService.getUser(matchedUserId)
                                if (userResponse.isSuccessful && userResponse.body() != null) {
                                    val user = userResponse.body()!!
                                    binding.selectedUserName.text = "Selected: ${user.name}"
                                    binding.btnDeleteUser.isEnabled = true
                                }
                            } catch (e: Exception) {
                                Log.e("MainActivity", "Failed to get user info: ${e.message}")
                            }
                        }
                    } else {
                        updateStatus("No matching user found", true)
                        Toast.makeText(this@MainActivity, "No matching user found.", Toast.LENGTH_LONG).show()
                    }
                }
            } catch (e: Exception) {
                Log.e("MainActivity", "Identification error: ${e.message}", e)
                runOnUiThread {
                    binding.btnIdentify.isEnabled = true
                    updateStatus("Error: ${e.message}", true)
                    Toast.makeText(this@MainActivity, "Error: ${e.message}", Toast.LENGTH_SHORT).show()
                }
            }
        }
    }
    
    private fun loadUsers() {
        lifecycleScope.launch {
            try {
                val response = ApiClient.apiService.listUsers()
                if (response.isSuccessful && response.body() != null) {
                    val users = response.body()!!
                    runOnUiThread {
                        userAdapter.submitList(users)
                        binding.userCountLabel.text = "Total users: ${users.size}"
                    }
                } else {
                    runOnUiThread {
                        updateStatus("Failed to load users: ${response.message()}", true)
                    }
                }
            } catch (e: Exception) {
                runOnUiThread {
                    updateStatus("Error loading users: ${e.message}", true)
                }
            }
        }
    }
    
    private fun deleteSelectedUser() {
        val userId = selectedUserId ?: run {
            Toast.makeText(this, "Please select a user first", Toast.LENGTH_SHORT).show()
            return
        }
        
        AlertDialog.Builder(this)
            .setTitle("Delete User")
            .setMessage("Are you sure you want to delete this user?")
            .setPositiveButton("Delete") { _, _ ->
                lifecycleScope.launch {
                    try {
                        val response = ApiClient.apiService.deleteUser(userId)
                        if (response.isSuccessful) {
                            runOnUiThread {
                                Toast.makeText(this@MainActivity, "User deleted", Toast.LENGTH_SHORT).show()
                                selectedUserId = null
                                binding.selectedUserName.text = "No user selected"
                                binding.btnDeleteUser.isEnabled = false
                                loadUsers()
                            }
                        } else {
                            runOnUiThread {
                                updateStatus("Failed to delete user", true)
                            }
                        }
                    } catch (e: Exception) {
                        runOnUiThread {
                            updateStatus("Error deleting user: ${e.message}", true)
                        }
                    }
                }
            }
            .setNegativeButton("Cancel", null)
            .show()
    }
    
    private fun enableControls(enabled: Boolean) {
        binding.btnStartVerify.isEnabled = enabled
        binding.btnIdentify.isEnabled = enabled
    }
    
    private fun updateStatus(message: String, isError: Boolean) {
        binding.statusLabel.text = "Status: $message"
        binding.statusLabel.setBackgroundColor(
            if (isError) 0xFFFFCCCB.toInt() else 0xFFE0F7FA.toInt()
        )
    }
    
    override fun onDestroy() {
        super.onDestroy()
        fingerprintManager.cleanup()
    }
}

