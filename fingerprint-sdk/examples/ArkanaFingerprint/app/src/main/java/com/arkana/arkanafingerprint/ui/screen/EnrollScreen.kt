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
import com.arkana.fingerprint.sdk.model.Finger

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun EnrollScreen(
    onNavigateBack: () -> Unit,
    onNavigateToUsers: () -> Unit,
    fingerprintViewModel: FingerprintViewModel = viewModel(),
    userViewModel: UserViewModel = viewModel()
) {
    var selectedUser by remember { mutableStateOf<User?>(null) }
    var selectedFinger by remember { mutableStateOf<Finger?>(Finger.LEFT_INDEX) }
    var showUserDropdown by remember { mutableStateOf(false) }
    
    val uiState by fingerprintViewModel.uiState.collectAsStateWithLifecycle()
    val enrollResult by fingerprintViewModel.enrollResult.collectAsStateWithLifecycle()
    val enrollmentProgress by fingerprintViewModel.enrollmentProgress.collectAsStateWithLifecycle()
    val userUiState by userViewModel.uiState.collectAsStateWithLifecycle()
    
    LaunchedEffect(Unit) {
        fingerprintViewModel.clearMessage()
        userViewModel.loadUsers()
    }
    
    LaunchedEffect(selectedUser) {
        selectedUser?.let { userViewModel.selectUser(it) }
    }
    
    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("Enroll Fingerprint") },
                navigationIcon = {
                    IconButton(onClick = onNavigateBack) {
                        Text("←", fontSize = 24.sp)
                    }
                },
                colors = TopAppBarDefaults.topAppBarColors(
                    containerColor = MaterialTheme.colorScheme.primaryContainer,
                    titleContentColor = MaterialTheme.colorScheme.onPrimaryContainer
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
                            MaterialTheme.colorScheme.primaryContainer
                        }
                    )
                ) {
                    Text(
                        text = message,
                        modifier = Modifier.padding(16.dp),
                        color = if (uiState.isError) {
                            MaterialTheme.colorScheme.onErrorContainer
                        } else {
                            MaterialTheme.colorScheme.onPrimaryContainer
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
                                            showUserDropdown = false
                                        }
                                    )
                                }
                            }
                        }
                    }
                    
                    if (selectedUser != null) {
                        Surface(
                            shape = MaterialTheme.shapes.small,
                            color = MaterialTheme.colorScheme.secondaryContainer
                        ) {
                            Text(
                                text = "${selectedUser!!.fingerCount} finger(s) enrolled",
                                modifier = Modifier.padding(horizontal = 12.dp, vertical = 6.dp),
                                style = MaterialTheme.typography.labelMedium
                            )
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
            Card(
                modifier = Modifier.fillMaxWidth(),
                elevation = CardDefaults.cardElevation(defaultElevation = 2.dp)
            ) {
                Column(
                    modifier = Modifier.padding(16.dp),
                    verticalArrangement = Arrangement.spacedBy(12.dp)
                ) {
                    Text(
                        text = "Finger",
                        style = MaterialTheme.typography.titleMedium,
                        fontWeight = FontWeight.SemiBold
                    )
                    
                    if (selectedUser != null && userUiState.userFingers.isNotEmpty()) {
                        Text(
                            text = "Enrolled: ${userUiState.userFingers.joinToString(", ")}",
                            style = MaterialTheme.typography.bodySmall,
                            color = MaterialTheme.colorScheme.primary
                        )
                    }
                    
                    val fingers = listOf(
                        Finger.LEFT_THUMB to "L. Thumb",
                        Finger.LEFT_INDEX to "L. Index",
                        Finger.LEFT_MIDDLE to "L. Middle",
                        Finger.LEFT_RING to "L. Ring",
                        Finger.LEFT_PINKY to "L. Pinky",
                        Finger.RIGHT_THUMB to "R. Thumb",
                        Finger.RIGHT_INDEX to "R. Index",
                        Finger.RIGHT_MIDDLE to "R. Middle",
                        Finger.RIGHT_RING to "R. Ring",
                        Finger.RIGHT_PINKY to "R. Pinky"
                    )
                    
                    Column(verticalArrangement = Arrangement.spacedBy(8.dp)) {
                        fingers.chunked(5).forEach { rowFingers ->
                            Row(
                                modifier = Modifier.fillMaxWidth(),
                                horizontalArrangement = Arrangement.spacedBy(8.dp)
                            ) {
                                rowFingers.forEach { (finger, label) ->
                                    val isEnrolled = selectedUser != null && 
                                        userUiState.userFingers.contains(finger.name)
                                    
                                    FilterChip(
                                        selected = selectedFinger == finger,
                                        onClick = { selectedFinger = finger },
                                        label = { Text(label, fontSize = 12.sp) },
                                        enabled = !uiState.isLoading && !isEnrolled,
                                        modifier = Modifier.weight(1f),
                                        colors = FilterChipDefaults.filterChipColors(
                                            selectedContainerColor = if (isEnrolled) {
                                                MaterialTheme.colorScheme.surfaceVariant
                                            } else {
                                                MaterialTheme.colorScheme.primaryContainer
                                            }
                                        )
                                    )
                                }
                            }
                        }
                    }
                }
            }
            
            // Enrollment Progress
            enrollmentProgress?.let { progress ->
                Card(
                    modifier = Modifier.fillMaxWidth(),
                    colors = CardDefaults.cardColors(
                        containerColor = MaterialTheme.colorScheme.secondaryContainer
                    )
                ) {
                    Column(
                        modifier = Modifier.padding(16.dp),
                        verticalArrangement = Arrangement.spacedBy(12.dp)
                    ) {
                        Text(
                            text = progress.message,
                            fontSize = 14.sp,
                            fontWeight = FontWeight.Medium
                        )
                        
                        LinearProgressIndicator(
                            progress = { progress.currentScan.toFloat() / progress.totalScans.toFloat() },
                            modifier = Modifier.fillMaxWidth().height(8.dp)
                        )
                        
                        Text(
                            text = "${progress.currentScan} / ${progress.totalScans}",
                            fontSize = 12.sp,
                            modifier = Modifier.align(Alignment.End)
                        )
                    }
                }
            }
            
            // Enroll Button
            Button(
                onClick = {
                    if (selectedUser != null && selectedFinger != null) {
                        fingerprintViewModel.enrollFingerprint(selectedUser!!.id, selectedFinger!!)
                    }
                },
                modifier = Modifier
                    .fillMaxWidth()
                    .height(56.dp),
                enabled = !uiState.isLoading && selectedUser != null && selectedFinger != null,
                colors = ButtonDefaults.buttonColors(
                    containerColor = MaterialTheme.colorScheme.primary
                )
            ) {
                if (uiState.isLoading) {
                    CircularProgressIndicator(
                        modifier = Modifier.size(24.dp),
                        color = MaterialTheme.colorScheme.onPrimary
                    )
                } else {
                    Text(
                        text = "Start Enrollment",
                        fontSize = 16.sp,
                        fontWeight = FontWeight.SemiBold
                    )
                }
            }
            
            // Success Result
            enrollResult?.let { result ->
                when (result) {
                    is com.arkana.fingerprint.sdk.model.EnrollResult.Success -> {
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
                                    text = "✓ Enrollment Successful",
                                    fontSize = 16.sp,
                                    fontWeight = FontWeight.Bold
                                )
                                Text(
                                    text = "User: ${selectedUser?.name ?: "Unknown"} (ID: ${result.userId})",
                                    fontSize = 14.sp
                                )
                                Text(
                                    text = "Finger: ${selectedFinger?.name ?: "Unknown"}",
                                    fontSize = 14.sp
                                )
                            }
                        }
                    }
                    is com.arkana.fingerprint.sdk.model.EnrollResult.Error -> {}
                }
            }
        }
    }
}
