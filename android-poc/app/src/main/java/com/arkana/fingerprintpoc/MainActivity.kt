package com.arkana.fingerprintpoc

import android.os.Bundle
import android.util.Base64
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
    private var enrollmentInProgress = false
    private var currentEnrollmentUserId: Int? = null
    private var enrollmentStages = 0
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
        
        // Enrollment
        binding.btnStartEnroll.setOnClickListener {
            startEnrollment()
        }
        
        // Verification
        binding.btnStartVerify.setOnClickListener {
            startVerification()
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
            updateStatus("Reader initialized successfully", false)
            enableControls(true)
            // Note: loadUsers() requires API server - comment out for UI demo
            // loadUsers()
        } else {
            updateStatus("Fingerprint hardware not available (Demo mode - UI only)", false)
            // Enable controls anyway for UI demo
            enableControls(true)
        }
        
        binding.btnInitialize.isEnabled = true
        binding.btnInitialize.text = "Initialize Reader"
    }
    
    private fun startEnrollment() {
        val name = binding.editEnrollName.text.toString().trim()
        val email = binding.editEnrollEmail.text.toString().trim()
        
        if (name.isEmpty()) {
            Toast.makeText(this, "Please enter a name", Toast.LENGTH_SHORT).show()
            return
        }
        
        if (!isInitialized) {
            Toast.makeText(this, "Please initialize reader first", Toast.LENGTH_SHORT).show()
            return
        }
        
        enrollmentInProgress = true
        binding.btnStartEnroll.isEnabled = false
        binding.enrollProgress.visibility = View.VISIBLE
        binding.enrollProgress.progress = 0
        
        lifecycleScope.launch {
            try {
                // Create user first
                val userResponse = ApiClient.apiService.createUser(
                    CreateUserRequest(name, email.ifEmpty { null })
                )
                
                if (userResponse.isSuccessful && userResponse.body() != null) {
                    val user = userResponse.body()!!
                    currentEnrollmentUserId = user.id
                    
                    // Start fingerprint enrollment
                    fingerprintManager.startEnrollment(
                        user.id,
                        onProgress = { current, total, message ->
                            runOnUiThread {
                                enrollmentStages = current
                                binding.enrollProgress.max = total
                                binding.enrollProgress.progress = current
                                binding.enrollStatusLabel.text = message
                            }
                        },
                        onComplete = { template ->
                            runOnUiThread {
                                // Upload template to server
                                lifecycleScope.launch {
                                    enrollFingerprintTemplate(user.id, template)
                                }
                            }
                        },
                        onError = { error ->
                            runOnUiThread {
                                enrollmentInProgress = false
                                binding.btnStartEnroll.isEnabled = true
                                binding.enrollProgress.visibility = View.GONE
                                updateStatus("Enrollment error: $error", true)
                            }
                        }
                    )
                } else {
                    throw IOException("Failed to create user: ${userResponse.message()}")
                }
            } catch (e: Exception) {
                enrollmentInProgress = false
                binding.btnStartEnroll.isEnabled = true
                binding.enrollProgress.visibility = View.GONE
                updateStatus("Error: ${e.message}", true)
            }
        }
    }
    
    private suspend fun enrollFingerprintTemplate(userId: Int, template: ByteArray) {
        try {
            val base64Template = Base64.encodeToString(template, Base64.NO_WRAP)
            val response = ApiClient.apiService.enrollFingerprint(
                userId,
                EnrollFingerprintRequest(base64Template)
            )
            
            runOnUiThread {
                enrollmentInProgress = false
                binding.btnStartEnroll.isEnabled = true
                binding.enrollProgress.visibility = View.GONE
                
                if (response.isSuccessful) {
                    updateStatus("Enrollment completed successfully!", false)
                    binding.editEnrollName.text?.clear()
                    binding.editEnrollEmail.text?.clear()
                    loadUsers()
                } else {
                    updateStatus("Failed to save fingerprint template", true)
                }
            }
        } catch (e: Exception) {
            runOnUiThread {
                enrollmentInProgress = false
                binding.btnStartEnroll.isEnabled = true
                binding.enrollProgress.visibility = View.GONE
                updateStatus("Error uploading template: ${e.message}", true)
            }
        }
    }
    
    private fun startVerification() {
        val userId = selectedUserId
        if (userId == null || userId <= 0) {
            Toast.makeText(this, "Please select a user first", Toast.LENGTH_SHORT).show()
            return
        }
        
        if (!isInitialized) {
            Toast.makeText(this, "Please initialize reader first", Toast.LENGTH_SHORT).show()
            return
        }
        
        binding.btnStartVerify.isEnabled = false
        updateStatus("Place your finger on the sensor...", false)
        
        lifecycleScope.launch {
            val (matched, score) = fingerprintManager.verifyFingerprint(userId)
            
            runOnUiThread {
                binding.btnStartVerify.isEnabled = true
                if (matched) {
                    updateStatus("Verification successful! Score: $score", false)
                    binding.verifyResultLabel.text = "MATCH - Score: $score"
                } else {
                    updateStatus("Verification failed - No match", true)
                    binding.verifyResultLabel.text = "NO MATCH"
                }
            }
        }
    }
    
    private fun startIdentification() {
        if (!isInitialized) {
            Toast.makeText(this, "Please initialize reader first", Toast.LENGTH_SHORT).show()
            return
        }
        
        binding.btnIdentify.isEnabled = false
        updateStatus("Identifying... Place finger on sensor", false)
        
        lifecycleScope.launch {
            // Get all user IDs first
            val usersResponse = ApiClient.apiService.listUsers()
            
            if (usersResponse.isSuccessful && usersResponse.body() != null) {
                val userIds = usersResponse.body()!!
                    .filter { it.hasFingerprint }
                    .map { it.id }
                
                if (userIds.isEmpty()) {
                    runOnUiThread {
                        binding.btnIdentify.isEnabled = true
                        updateStatus("No enrolled users found", true)
                    }
                    return@launch
                }
                
                val (matchedUserId, score) = fingerprintManager.identifyUser(userIds)
                
                runOnUiThread {
                    binding.btnIdentify.isEnabled = true
                    if (matchedUserId > 0) {
                        updateStatus("User identified! ID: $matchedUserId, Score: $score", false)
                        loadUsers() // Refresh to highlight matched user
                    } else {
                        updateStatus("No matching user found", true)
                    }
                }
            } else {
                runOnUiThread {
                    binding.btnIdentify.isEnabled = true
                    updateStatus("Failed to load users", true)
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
        binding.btnStartEnroll.isEnabled = enabled
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

