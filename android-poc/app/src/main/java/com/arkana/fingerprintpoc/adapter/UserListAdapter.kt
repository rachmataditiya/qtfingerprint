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
    
    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): UserViewHolder {
        val binding = ItemUserBinding.inflate(
            LayoutInflater.from(parent.context),
            parent,
            false
        )
        return UserViewHolder(binding, onUserSelected)
    }
    
    override fun onBindViewHolder(holder: UserViewHolder, position: Int) {
        holder.bind(getItem(position))
    }
    
    class UserViewHolder(
        private val binding: ItemUserBinding,
        private val onUserSelected: (UserResponse) -> Unit
    ) : RecyclerView.ViewHolder(binding.root) {
        
        fun bind(user: UserResponse) {
            binding.textUserName.text = user.name
            binding.textUserEmail.text = user.email ?: "No email"
            binding.textUserId.text = "ID: ${user.id}"
            binding.textHasFingerprint.text = if (user.hasFingerprint) "✓ Enrolled" else "✗ Not enrolled"
            
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

