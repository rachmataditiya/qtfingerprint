package com.arkana.fingerprintpoc

import android.app.PendingIntent
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.hardware.usb.UsbDevice
import android.hardware.usb.UsbDeviceConnection
import android.hardware.usb.UsbManager
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
import com.arkana.libdigitalpersona.FingerprintJNI
import kotlinx.coroutines.launch

class MainActivity : AppCompatActivity() {
    
    private lateinit var binding: ActivityMainBinding
    private lateinit var fingerprintManager: FingerprintManager
    private lateinit var usbManager: UsbManager
    private lateinit var userListAdapter: com.arkana.fingerprintpoc.adapter.UserListAdapter
    
    private var isInitialized = false
    private var usbDevice: UsbDevice? = null
    private var selectedUserId: Int? = null
    
    private val usbPermissionReceiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context, intent: Intent) {
            if (intent.action == "android.hardware.usb.action.USB_PERMISSION") {
                synchronized(this) {
                    val device: UsbDevice? = intent.getParcelableExtra(UsbManager.EXTRA_DEVICE)
                    if (intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false)) {
                        device?.apply {
                            Log.d("MainActivity", "USB permission granted for device: ${device.deviceName}")
                            usbDevice = device
                            // Now initialize libfprint after permission granted
                            initializeLibfprint()
                        }
                    } else {
                        Log.e("MainActivity", "USB permission denied for device: ${device?.deviceName}")
                        runOnUiThread {
                            updateStatus("USB permission denied", true)
                            binding.btnInitialize.isEnabled = true
                            binding.btnInitialize.text = "Initialize USB Device"
                        }
                    }
                }
            }
        }
    }
    
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)
        
        usbManager = getSystemService(Context.USB_SERVICE) as UsbManager
        fingerprintManager = FingerprintManager(this)
        
        // Register USB permission receiver
        // For Android 13+ (API 33+), we need to specify RECEIVER_EXPORTED or RECEIVER_NOT_EXPORTED
        val filter = IntentFilter("android.hardware.usb.action.USB_PERMISSION")
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.TIRAMISU) {
            registerReceiver(usbPermissionReceiver, filter, android.content.Context.RECEIVER_NOT_EXPORTED)
        } else {
            @Suppress("DEPRECATION")
            registerReceiver(usbPermissionReceiver, filter)
        }
        
        setupUI()
        setupUserList()
        
        // Check if USB device was attached via intent
        handleUsbIntent(intent)
        
        // Load user list on startup
        loadUserList()
    }
    
    override fun onNewIntent(intent: Intent) {
        super.onNewIntent(intent)
        handleUsbIntent(intent)
    }
    
    private fun handleUsbIntent(intent: Intent?) {
        if (intent?.action == "android.hardware.usb.action.USB_DEVICE_ATTACHED") {
            val device: UsbDevice? = intent.getParcelableExtra(UsbManager.EXTRA_DEVICE)
            device?.let {
                Log.d("MainActivity", "USB device attached: ${it.deviceName}")
                requestUsbPermission(it)
            }
        }
    }
    
    private fun requestUsbPermission(device: UsbDevice) {
        if (usbManager.hasPermission(device)) {
            Log.d("MainActivity", "USB permission already granted")
            usbDevice = device
            initializeLibfprint()
        } else {
            Log.d("MainActivity", "Requesting USB permission for device: ${device.deviceName}")
            val permissionIntent = PendingIntent.getBroadcast(
                this, 0,
                Intent("android.hardware.usb.action.USB_PERMISSION"),
                PendingIntent.FLAG_IMMUTABLE
            )
            usbManager.requestPermission(device, permissionIntent)
        }
    }
    
    private fun setupUI() {
        // Initialize USB Device button
        binding.btnInitialize.setOnClickListener {
            initializeReader()
        }
        
        // Verification button - use selected user from list
        binding.btnStartVerify.setOnClickListener {
            if (!isInitialized) {
                Toast.makeText(this, "Please initialize USB device first", Toast.LENGTH_SHORT).show()
                return@setOnClickListener
            }
            
            if (selectedUserId == null) {
                Toast.makeText(this, "Please select a user from the list first", Toast.LENGTH_SHORT).show()
                return@setOnClickListener
            }
            
            startVerification(selectedUserId!!)
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
        enableControls(false)
        hideResult()
    }
    
    private fun initializeReader() {
        try {
            binding.btnInitialize.isEnabled = false
            binding.btnInitialize.text = "Initializing..."
            updateStatus("Checking USB devices...", false)
            
            // Find USB fingerprint device
            val deviceList = usbManager.deviceList
            val fingerprintDevice = deviceList.values.find { device ->
                device.vendorId == 1466 && device.productId == 10 // DigitalPersona U.are.U 4500
            }
            
            if (fingerprintDevice == null) {
                updateStatus("USB fingerprint reader not found - Please connect device", true)
                enableControls(false)
                binding.btnInitialize.text = "Initialize USB Device"
                binding.btnInitialize.isEnabled = true
                return
            }
            
            Log.d("MainActivity", "Found USB device: ${fingerprintDevice.deviceName}")
            requestUsbPermission(fingerprintDevice)
        } catch (e: Exception) {
            Log.e("MainActivity", "Error in initializeReader: ${e.message}", e)
            e.printStackTrace()
            runOnUiThread {
                updateStatus("Error: ${e.message}", true)
                binding.btnInitialize.isEnabled = true
                binding.btnInitialize.text = "Initialize USB Device"
            }
        }
    }
    
    private fun initializeLibfprint() {
        if (usbDevice == null) {
            Log.e("MainActivity", "USB device is null")
            updateStatus("USB device not found", true)
            binding.btnInitialize.isEnabled = true
            binding.btnInitialize.text = "Initialize USB Device"
            return
        }
        
        // Open USB device using Android USB Host API
        val connection = usbManager.openDevice(usbDevice)
        if (connection == null) {
            Log.e("MainActivity", "Failed to open USB device")
            updateStatus("Failed to open USB device", true)
            binding.btnInitialize.isEnabled = true
            binding.btnInitialize.text = "Initialize USB Device"
            return
        }
        
        // Get file descriptor from USB connection
        val fd = connection.fileDescriptor
        if (fd < 0) {
            Log.e("MainActivity", "Invalid file descriptor: $fd")
            connection.close()
            updateStatus("Failed to get USB file descriptor", true)
            binding.btnInitialize.isEnabled = true
            binding.btnInitialize.text = "Initialize USB Device"
            return
        }
        
        Log.d("MainActivity", "USB device opened, file descriptor: $fd")
        
        // Initialize libfprint first (this creates the native instance)
        val initialized = FingerprintJNI.initialize(this)
        if (!initialized) {
            Log.e("MainActivity", "Failed to initialize FingerprintJNI: ${FingerprintJNI.getLastError()}")
            connection.close()
            updateStatus("Failed to initialize libfprint", true)
            binding.btnInitialize.isEnabled = true
            binding.btnInitialize.text = "Initialize USB Device"
            return
        }
        
        Log.d("MainActivity", "FingerprintJNI initialized successfully")
        
        // Now pass file descriptor to native code
        val fdSet = FingerprintJNI.setUsbFileDescriptor(fd)
        if (!fdSet) {
            Log.e("MainActivity", "Failed to set USB file descriptor: ${FingerprintJNI.getLastError()}")
            connection.close()
            FingerprintJNI.cleanup()
            updateStatus("Failed to set USB file descriptor", true)
            binding.btnInitialize.isEnabled = true
            binding.btnInitialize.text = "Initialize USB Device"
            return
        }
        
        Log.d("MainActivity", "USB file descriptor set successfully")
        
        // Now initialize fingerprint manager (which will use the file descriptor)
        val managerInitialized = fingerprintManager.initialize(this)
        if (managerInitialized) {
            isInitialized = true
            updateStatus("USB device initialized successfully", false)
            enableControls(true)
            binding.btnInitialize.text = "Device Ready"
        } else {
            updateStatus("Failed to initialize libfprint: ${fingerprintManager.getLastError()}", true)
            enableControls(false)
            binding.btnInitialize.text = "Initialize USB Device"
        }
        
        binding.btnInitialize.isEnabled = true
        
        // Keep connection open (don't close it)
        // The file descriptor needs to remain valid for libusb
    }
    
    private fun initializeLibfprintOld() {
        updateStatus("Initializing libfprint...", false)
        
        val available = fingerprintManager.initialize(this)
        
        if (available) {
            isInitialized = true
            updateStatus("USB device initialized successfully", false)
            enableControls(true)
            binding.btnInitialize.text = "Device Ready"
        } else {
            updateStatus("Failed to initialize libfprint: ${fingerprintManager.getLastError()}", true)
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
    
    // Removed updateUserInfo - not needed for PoC
    
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
        try {
            unregisterReceiver(usbPermissionReceiver)
        } catch (e: Exception) {
            // Receiver might not be registered
        }
        fingerprintManager.cleanup()
    }
    
    private fun getLastError(): String {
        return fingerprintManager.getLastError()
    }
    
    private fun setupUserList() {
        try {
            userListAdapter = com.arkana.fingerprintpoc.adapter.UserListAdapter { user ->
                selectedUserId = user.id
                userListAdapter.setSelectedUserId(user.id)
                Toast.makeText(this, "Selected user: ${user.name} (ID: ${user.id})", Toast.LENGTH_SHORT).show()
                Log.d("MainActivity", "User selected: ${user.name} (ID: ${user.id})")
            }
            
            binding.recyclerViewUsers.layoutManager = androidx.recyclerview.widget.LinearLayoutManager(this)
            binding.recyclerViewUsers.adapter = userListAdapter
            // Enable smooth scrolling
            binding.recyclerViewUsers.setHasFixedSize(false)
            Log.d("MainActivity", "User list setup completed successfully")
        } catch (e: Exception) {
            Log.e("MainActivity", "Error setting up user list: ${e.message}", e)
            e.printStackTrace()
        }
    }
    
    private fun loadUserList() {
        lifecycleScope.launch {
            try {
                val response = ApiClient.apiService.listUsers()
                if (response.isSuccessful && response.body() != null) {
                    val users = response.body()!!
                    runOnUiThread {
                        userListAdapter.submitList(users)
                        Log.d("MainActivity", "Loaded ${users.size} users")
                    }
                } else {
                    Log.e("MainActivity", "Failed to load users: ${response.message()}")
                    runOnUiThread {
                        Toast.makeText(this@MainActivity, "Failed to load users: ${response.message()}", Toast.LENGTH_SHORT).show()
                    }
                }
            } catch (e: Exception) {
                Log.e("MainActivity", "Error loading users: ${e.message}", e)
                runOnUiThread {
                    Toast.makeText(this@MainActivity, "Error loading users: ${e.message}", Toast.LENGTH_SHORT).show()
                }
            }
        }
    }
}
