#pragma once

#include <string>
#include <eosiolib/crypto.hpp>
#include <eosiolib/time.hpp>
#include <eosio.token/eosio.token.hpp>

class [[eosio::contract]] eos_htlc : public eosio::contract
{
   public:
      /*****
       * default constructor
       */
      eos_htlc(eosio::name receiver, eosio::name code, eosio::datastream<const char*> ds)
            :contract(receiver, code, ds) {}

      /****
       * Create a new HTLC
       */
      [[eosio::action]]
      uint64_t create(eosio::name sender, eosio::name receiver, eosio::asset token, 
            eosio::checksum256 hashlock, eosio::time_point timelock);

      /*****
       * I have the preimage. Send the tokens to the receiver
       */
      [[eosio::action]]
      void withdraw(eosio::checksum256 id, std::string preimage);

      /*****
       * Return the tokens to the sender
       */
      [[eosio::action]]
      void refund(eosio::checksum256 id);

   private:

      /***
       * persistence record format
       */
      struct [[eosio::table]] htlc_contract
      {
         uint64_t key; // unique key
         eosio::checksum256 id;
         eosio::name sender; // who created the HTLC
         eosio::name receiver; // the destination for the tokens
         eosio::asset token; // the token and quantity
         eosio::checksum256 hashlock; // the hash of the preimage
         eosio::time_point timelock; // when the contract expires and sender can ask for refund
         bool withdrawn; // true if receiver provides the preimage
         bool refunded; // true if sender is refunded
         std::string preimage; /// the preimage provided by the receiver to claim

         uint64_t primary_key() const { return key; }
         eosio::checksum256 by_id() const { return id; }

         htlc_contract() {}
         htlc_contract(eosio::name sender, eosio::name receiver, eosio::asset token,
               eosio::checksum256 hashlock, eosio::time_point timelock)
         {
            this->sender = sender;
            this->receiver = receiver;
            this->token = token;
            this->hashlock = hashlock;
            this->timelock = timelock;
            this->preimage = "";
            this->withdrawn = false;
            this->refunded = false;
            this->id = create_id(this); // unique secondary index
            // NOTE: this->key is set elsewhere
         }
      };

      /*****
       * Indexing
       */
      typedef eosio::multi_index<"htlcs"_n, htlc_contract,
            eosio::indexed_by<"id"_n, 
            eosio::const_mem_fun<htlc_contract, eosio::checksum256, 
            &htlc_contract::by_id>>> htlc_index;

      /****
       * Assists in building the id. This should only be called by the ctor, as some fields
       * must be set to the default for hashing to work correctly
       * 
       * @param in the contract to generate the id from
       * @param the sha256 hash of the contract
       */
      static eosio::checksum256 create_id( htlc_contract* const in)
      {
         return eosio::sha256( reinterpret_cast<char *>(in), sizeof(htlc_contract)  );
      }

      /***
       * Get a contract by its id
       */
      std::shared_ptr<eos_htlc::htlc_contract> get_by_id(eosio::checksum256 id); 
};