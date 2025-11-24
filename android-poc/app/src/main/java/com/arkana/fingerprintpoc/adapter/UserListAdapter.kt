package com.arkana.fingerprintpoc.adapter

import android.view.LayoutInflater
import android.view.ViewGroup
import androidx.recyclerview.widget.DiffUtil
import androidx.recyclerview.widget.ListAdapter
import androidx.recyclerview.widget.RecyclerView
import com.arkana.fingerprintpoc.databinding.ItemUserBinding
import com.arkana.fingerprintpoc.models.UserResponse

class UserListAdapter(
    private val onUserSelected: (UserResponse) -> Unit
) : ListAdapter<UserResponse, UserListAdapter.UserViewHolder>(UserDiffCallback()) {
    
    private var selectedUserId: Int? = null
    
    fun setSelectedUserId(userId: Int?) {
        val previousSelected = selectedUserId
        selectedUserId = userId
        // Notify changes to update UI
        if (previousSelected != null) {
            val previousPosition = currentList.indexOfFirst { it.id == previousSelected }
            if (previousPosition >= 0) {
                notifyItemChanged(previousPosition)
            }
        }
        if (userId != null) {
            val newPosition = currentList.indexOfFirst { it.id == userId }
            if (newPosition >= 0) {
                notifyItemChanged(newPosition)
            }
        }
    }
    
    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): UserViewHolder {
        val binding = ItemUserBinding.inflate(
            LayoutInflater.from(parent.context),
            parent,
            false
        )
        return UserViewHolder(binding, onUserSelected)
    }
    
    override fun onBindViewHolder(holder: UserViewHolder, position: Int) {
        val user = getItem(position)
        val isSelected = user.id == selectedUserId
        holder.bind(user, isSelected)
    }
    
    class UserViewHolder(
        private val binding: ItemUserBinding,
        private val onUserSelected: (UserResponse) -> Unit
    ) : RecyclerView.ViewHolder(binding.root) {
        
        fun bind(user: UserResponse, isSelected: Boolean) {
            binding.textUserName.text = user.name
            binding.textUserEmail.text = user.email ?: "No email"
            binding.textUserId.text = "ID: ${user.id}"
            binding.textHasFingerprint.text = if (user.hasFingerprint) "✓ Enrolled" else "✗ Not enrolled"
            
            // Update visual state based on selection
            if (isSelected) {
                binding.root.setCardBackgroundColor(binding.root.context.getColor(android.R.color.holo_blue_light))
                binding.textUserName.setTextColor(binding.root.context.getColor(android.R.color.white))
                binding.textUserEmail.setTextColor(binding.root.context.getColor(android.R.color.white))
                binding.textUserId.setTextColor(binding.root.context.getColor(android.R.color.white))
                binding.textHasFingerprint.setTextColor(binding.root.context.getColor(android.R.color.white))
                binding.root.cardElevation = 8f
            } else {
                binding.root.setCardBackgroundColor(binding.root.context.getColor(android.R.color.white))
                binding.textUserName.setTextColor(binding.root.context.getColor(android.R.color.black))
                binding.textUserEmail.setTextColor(binding.root.context.getColor(android.R.color.darker_gray))
                binding.textUserId.setTextColor(binding.root.context.getColor(android.R.color.darker_gray))
                binding.textHasFingerprint.setTextColor(
                    if (user.hasFingerprint) 
                        binding.root.context.getColor(android.R.color.holo_green_dark)
                    else 
                        binding.root.context.getColor(android.R.color.darker_gray)
                )
                binding.root.cardElevation = 2f
            }
            
            binding.root.setOnClickListener {
                onUserSelected(user)
            }
        }
    }
    
    class UserDiffCallback : DiffUtil.ItemCallback<UserResponse>() {
        override fun areItemsTheSame(oldItem: UserResponse, newItem: UserResponse): Boolean {
            return oldItem.id == newItem.id
        }
        
        override fun areContentsTheSame(oldItem: UserResponse, newItem: UserResponse): Boolean {
            return oldItem == newItem
        }
    }
}

