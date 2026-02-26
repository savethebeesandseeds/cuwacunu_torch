# Jkimyei means effort, here it means learning

This folder contains the training structures and methods, the learning.



/*
  
  
  learning:
  Curriculum Learning: first learn from full data, then learn from masks
  dont use droput or standard variation
  - L2 regularization is alright
  - srink and perturb is also alright
  - stochastic depth
  Continual Backpropagation (Richard Sutton):
    - inspecting the network for the neurons that are beeing activated (utility mesarures) the less and reinitializing their parameters is nice. 
      
  Architrecute:
  - use of residual networks ResNets


  note, maybe usefull padded sequences
  auto packed_input = torch::nn::utils::rnn::pack_padded_sequence(inputs, lengths, batch_first=>true);
*/