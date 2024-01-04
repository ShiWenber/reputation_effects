# ./build/reputation_effects --payoff_matrix_config_name payoffMatrix_shortterm --stepNum 10000 --logStep 1 --threads 8 --population 100 --p0 1 --episode 100000;
# ./build/reputation_effects --payoff_matrix_config_name payoffMatrix_shortterm --stepNum 10000 --logStep 1 --threads 8 --population 100 --p0 0 --episode 100000;
# ./build/reputation_effects --payoff_matrix_config_name payoffMatrix_shortterm --stepNum 10000 --logStep 1 --threads 8 --population 100 --p0 0.2 --episode 100000;
# ./build/reputation_effects --payoff_matrix_config_name payoffMatrix_shortterm --stepNum 10000 --logStep 1 --threads 8 --population 100 --p0 0.5 --episode 100000;
# ./build/reputation_effects --payoff_matrix_config_name payoffMatrix_shortterm --stepNum 10000 --logStep 1 --threads 8 --population 100 --p0 0.55 --episode 100000;
# ./build/reputation_effects --payoff_matrix_config_name payoffMatrix_shortterm --stepNum 10000 --logStep 1 --threads 8 --population 100 --p0 0.8 --episode 100000 --epsilon 0.001;



# ./build/reputation_effects --payoff_matrix_config_name payoffMatrix_longterm_no_norm_error --stepNum 2000 --episode 40000 --alpha 0.01 --discount 0.99 --epsilon 0.1 --logStep 40 --threads 8 --population 100 --p0 1;
./build/reputation_effects --payoff_matrix_config_name payoffMatrix_longterm_no_norm_error --stepNum 2000 --episode 40000 --alpha 0.01 --discount 0.99 --epsilon 0.1 --logStep 40 --threads 8 --population 100 --p0 1 --with_boltzmann true --beta_boltzmann 2;