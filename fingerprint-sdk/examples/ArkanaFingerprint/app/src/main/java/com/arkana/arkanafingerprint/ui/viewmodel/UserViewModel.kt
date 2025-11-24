package com.arkana.arkanafingerprint.ui.viewmodel

import android.app.Application
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import com.arkana.arkanafingerprint.data.PreferencesManager
import com.arkana.arkanafingerprint.data.model.User
import com.arkana.fingerprint.sdk.backend.BackendClient
import com.arkana.fingerprint.sdk.backend.BackendResult
import com.arkana.fingerprint.sdk.backend.UserInfo
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch

data class UserUiState(
    val isLoading: Boolean = false,
    val message: String? = null,
    val isError: Boolean = false,
    val users: List<User> = emptyList(),
    val selectedUser: User? = null,
    val userFingers: List<String> = emptyList()
)

class UserViewModel(application: Application) : AndroidViewModel(application) {
    private val preferencesManager = PreferencesManager(application)
    private val backendClient = BackendClient(preferencesManager.backendUrl)
    
    private val _uiState = MutableStateFlow(UserUiState())
    val uiState: StateFlow<UserUiState> = _uiState.asStateFlow()

    init {
        loadUsers()
    }

    fun loadUsers() {
        viewModelScope.launch {
            _uiState.value = _uiState.value.copy(isLoading = true, message = "Loading users...")
            try {
                val result = backendClient.listUsers()
                when (result) {
                    is BackendResult.Users -> {
                        val users = result.users.map { userInfo ->
                            User(
                                id = userInfo.id,
                                name = userInfo.name,
                                email = userInfo.email,
                                fingerCount = userInfo.fingerCount
                            )
                        }
                        _uiState.value = _uiState.value.copy(
                            isLoading = false,
                            users = users,
                            message = null
                        )
                    }
                    is BackendResult.Error -> {
                        _uiState.value = _uiState.value.copy(
                            isLoading = false,
                            isError = true,
                            message = result.message ?: "Failed to load users"
                        )
                    }
                    else -> {
                        _uiState.value = _uiState.value.copy(
                            isLoading = false,
                            isError = true,
                            message = "Unexpected response from backend"
                        )
                    }
                }
            } catch (e: Exception) {
                _uiState.value = _uiState.value.copy(
                    isLoading = false,
                    isError = true,
                    message = "Error loading users: ${e.message}"
                )
            }
        }
    }

    fun createUser(name: String, email: String?) {
        viewModelScope.launch {
            _uiState.value = _uiState.value.copy(isLoading = true, message = "Creating user...")
            try {
                val result = backendClient.createUser(name, email)
                when (result) {
                    is BackendResult.UserCreated -> {
                        _uiState.value = _uiState.value.copy(
                            isLoading = false,
                            message = "User created successfully"
                        )
                        loadUsers() // Reload users list
                    }
                    is BackendResult.Error -> {
                        _uiState.value = _uiState.value.copy(
                            isLoading = false,
                            isError = true,
                            message = result.message ?: "Failed to create user"
                        )
                    }
                    else -> {
                        _uiState.value = _uiState.value.copy(
                            isLoading = false,
                            isError = true,
                            message = "Unexpected response from backend"
                        )
                    }
                }
            } catch (e: Exception) {
                _uiState.value = _uiState.value.copy(
                    isLoading = false,
                    isError = true,
                    message = "Error creating user: ${e.message}"
                )
            }
        }
    }

    fun selectUser(user: User) {
        viewModelScope.launch {
            _uiState.value = _uiState.value.copy(
                selectedUser = user,
                isLoading = true,
                message = "Loading user fingers..."
            )
            try {
                val result = backendClient.getUserFingers(user.id)
                when (result) {
                    is BackendResult.Fingers -> {
                        _uiState.value = _uiState.value.copy(
                            isLoading = false,
                            userFingers = result.fingers,
                            message = null
                        )
                    }
                    is BackendResult.Error -> {
                        _uiState.value = _uiState.value.copy(
                            isLoading = false,
                            userFingers = emptyList(),
                            message = result.message ?: "Failed to load user fingers"
                        )
                    }
                    else -> {
                        _uiState.value = _uiState.value.copy(
                            isLoading = false,
                            userFingers = emptyList()
                        )
                    }
                }
            } catch (e: Exception) {
                _uiState.value = _uiState.value.copy(
                    isLoading = false,
                    userFingers = emptyList(),
                    message = "Error loading user fingers: ${e.message}"
                )
            }
        }
    }

    fun clearMessage() {
        _uiState.value = _uiState.value.copy(message = null, isError = false)
    }
}

