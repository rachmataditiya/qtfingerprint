package com.arkana.arkanafingerprint.ui.screen

import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.ArrowDropDown
import androidx.compose.material.icons.filled.Person
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import androidx.lifecycle.viewmodel.compose.viewModel
import com.arkana.arkanafingerprint.data.model.User
import com.arkana.arkanafingerprint.ui.viewmodel.FingerprintViewModel
import com.arkana.arkanafingerprint.ui.viewmodel.UserViewModel

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun VerifyScreen(
    onNavigateBack: () -> Unit,
    onNavigateToUsers: () -> Unit,
    fingerprintViewModel: FingerprintViewModel = viewModel(),
    userViewModel: UserViewModel = viewModel()
) {
    var selectedUser by remember { mutableStateOf<User?>(null) }
    var selectedFinger by remember { mutableStateOf<String?>(null) }
    var showUserDropdown by remember { mutableStateOf(false) }
    var showFingerDropdown by remember { mutableStateOf(false) }
    
    val uiState by fingerprintViewModel.uiState.collectAsStateWithLifecycle()
    val verifyResult by fingerprintViewModel.verifyResult.collectAsStateWithLifecycle()
    val userUiState by userViewModel.uiState.collectAsStateWithLifecycle()
    
    LaunchedEffect(Unit) {
        fingerprintViewModel.clearMessage()
        userViewModel.loadUsers()
    }
    
    LaunchedEffect(selectedUser) {
        selectedFinger = null
        if (selectedUser != null) {
            userViewModel.selectUser(selectedUser!!)
        }
    }
    
    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("Verify Fingerprint") },
                navigationIcon = {
                    IconButton(onClick = onNavigateBack) {
                        Text("←", fontSize = 24.sp)
                    }
                },
                colors = TopAppBarDefaults.topAppBarColors(
                    containerColor = MaterialTheme.colorScheme.secondaryContainer,
                    titleContentColor = MaterialTheme.colorScheme.onSecondaryContainer
                )
            )
        }
    ) { paddingValues ->
        Column(
            modifier = Modifier
                .fillMaxSize()
                .padding(paddingValues)
                .verticalScroll(rememberScrollState())
                .padding(horizontal = 24.dp, vertical = 16.dp),
            verticalArrangement = Arrangement.spacedBy(16.dp)
        ) {
            // Status Card
            val message = uiState.message
            if (message != null) {
                Card(
                    modifier = Modifier.fillMaxWidth(),
                    colors = CardDefaults.cardColors(
                        containerColor = if (uiState.isError) {
                            MaterialTheme.colorScheme.errorContainer
                        } else {
                            MaterialTheme.colorScheme.secondaryContainer
                        }
                    )
                ) {
                    Text(
                        text = message,
                        modifier = Modifier.padding(16.dp),
                        color = if (uiState.isError) {
                            MaterialTheme.colorScheme.onErrorContainer
                        } else {
                            MaterialTheme.colorScheme.onSecondaryContainer
                        }
                    )
                }
            }
            
            // User Selection
            Card(
                modifier = Modifier.fillMaxWidth(),
                elevation = CardDefaults.cardElevation(defaultElevation = 2.dp)
            ) {
                Column(
                    modifier = Modifier.padding(16.dp),
                    verticalArrangement = Arrangement.spacedBy(12.dp)
                ) {
                    Text(
                        text = "User",
                        style = MaterialTheme.typography.titleMedium,
                        fontWeight = FontWeight.SemiBold
                    )
                    
                    Box {
                        OutlinedTextField(
                            value = selectedUser?.let { "${it.name}${it.email?.let { " ($it)" } ?: ""}" } ?: "",
                            onValueChange = { },
                            readOnly = true,
                            placeholder = { Text("Select user") },
                            modifier = Modifier
                                .fillMaxWidth()
                                .clickable { showUserDropdown = true },
                            trailingIcon = {
                                IconButton(onClick = { showUserDropdown = true }) {
                                    Icon(Icons.Default.ArrowDropDown, contentDescription = null)
                                }
                            },
                            leadingIcon = {
                                Icon(Icons.Default.Person, contentDescription = null)
                            },
                            enabled = !uiState.isLoading
                        )
                        
                        DropdownMenu(
                            expanded = showUserDropdown,
                            onDismissRequest = { showUserDropdown = false },
                            modifier = Modifier.fillMaxWidth()
                        ) {
                            if (userUiState.users.isEmpty()) {
                                DropdownMenuItem(
                                    text = { Text("No users available") },
                                    onClick = { showUserDropdown = false }
                                )
                            } else {
                                userUiState.users.forEach { user ->
                                    DropdownMenuItem(
                                        text = {
                                            Column {
                                                Text(text = user.name, fontWeight = FontWeight.SemiBold)
                                                user.email?.let {
                                                    Text(
                                                        text = it,
                                                        style = MaterialTheme.typography.bodySmall,
                                                        color = MaterialTheme.colorScheme.onSurfaceVariant
                                                    )
                                                }
                                            }
                                        },
                                        onClick = {
                                            selectedUser = user
                                            selectedFinger = null
                                            showUserDropdown = false
                                            userViewModel.selectUser(user)
                                        }
                                    )
                                }
                            }
                        }
                    }
                    
                    TextButton(
                        onClick = onNavigateToUsers,
                        modifier = Modifier.fillMaxWidth()
                    ) {
                        Icon(Icons.Default.Person, contentDescription = null)
                        Spacer(Modifier.width(8.dp))
                        Text("Manage Users")
                    }
                }
            }
            
            // Finger Selection
            if (selectedUser != null && userUiState.userFingers.isNotEmpty()) {
                Card(
                    modifier = Modifier.fillMaxWidth(),
                    elevation = CardDefaults.cardElevation(defaultElevation = 2.dp)
                ) {
                    Column(
                        modifier = Modifier.padding(16.dp),
                        verticalArrangement = Arrangement.spacedBy(12.dp)
                    ) {
                        Text(
                            text = "Finger (Optional)",
                            style = MaterialTheme.typography.titleMedium,
                            fontWeight = FontWeight.SemiBold
                        )
                        
                        Box {
                            OutlinedTextField(
                                value = selectedFinger ?: "",
                                onValueChange = { },
                                readOnly = true,
                                placeholder = { Text("Most recent") },
                                modifier = Modifier
                                    .fillMaxWidth()
                                    .clickable { showFingerDropdown = true },
                                trailingIcon = {
                                    IconButton(onClick = { showFingerDropdown = true }) {
                                        Icon(Icons.Default.ArrowDropDown, contentDescription = null)
                                    }
                                },
                                enabled = !uiState.isLoading && !userUiState.isLoading
                            )
                            
                            DropdownMenu(
                                expanded = showFingerDropdown,
                                onDismissRequest = { showFingerDropdown = false },
                                modifier = Modifier.fillMaxWidth()
                            ) {
                                DropdownMenuItem(
                                    text = { Text("Most Recent") },
                                    onClick = {
                                        selectedFinger = null
                                        showFingerDropdown = false
                                    }
                                )
                                userUiState.userFingers.forEach { finger ->
                                    DropdownMenuItem(
                                        text = { Text(finger) },
                                        onClick = {
                                            selectedFinger = finger
                                            showFingerDropdown = false
                                        }
                                    )
                                }
                            }
                        }
                    }
                }
            }
            
            // Verify Button
            Button(
                onClick = {
                    if (selectedUser != null) {
                        val finger = selectedFinger?.let { 
                            try {
                                com.arkana.fingerprint.sdk.model.Finger.valueOf(it)
                            } catch (e: Exception) {
                                null
                            }
                        }
                        fingerprintViewModel.verifyFingerprint(selectedUser!!.id, finger)
                    }
                },
                modifier = Modifier
                    .fillMaxWidth()
                    .height(56.dp),
                enabled = !uiState.isLoading && selectedUser != null && !userUiState.isLoading,
                colors = ButtonDefaults.buttonColors(
                    containerColor = MaterialTheme.colorScheme.secondary
                )
            ) {
                if (uiState.isLoading) {
                    CircularProgressIndicator(
                        modifier = Modifier.size(24.dp),
                        color = MaterialTheme.colorScheme.onPrimary
                    )
                } else {
                    Text(
                        text = "Verify Fingerprint",
                        fontSize = 16.sp,
                        fontWeight = FontWeight.SemiBold
                    )
                }
            }
            
            // Result Card
            verifyResult?.let { result ->
                when (result) {
                    is com.arkana.fingerprint.sdk.model.VerifyResult.Success -> {
                        Card(
                            modifier = Modifier.fillMaxWidth(),
                            colors = CardDefaults.cardColors(
                                containerColor = MaterialTheme.colorScheme.primaryContainer
                            )
                        ) {
                            Column(
                                modifier = Modifier.padding(16.dp),
                                verticalArrangement = Arrangement.spacedBy(8.dp)
                            ) {
                                Text(
                                    text = "✓ Verification Successful",
                                    fontSize = 16.sp,
                                    fontWeight = FontWeight.Bold
                                )
                                Text(
                                    text = "User: ${selectedUser?.name ?: "Unknown"} (ID: ${result.userId})",
                                    fontSize = 14.sp
                                )
                                if (selectedFinger != null) {
                                    Text(
                                        text = "Finger: $selectedFinger",
                                        fontSize = 14.sp
                                    )
                                }
                                Text(
                                    text = "Score: ${String.format("%.2f", result.score)}",
                                    fontSize = 14.sp,
                                    fontWeight = FontWeight.SemiBold
                                )
                            }
                        }
                    }
                    is com.arkana.fingerprint.sdk.model.VerifyResult.Error -> {}
                }
            }
        }
    }
}
