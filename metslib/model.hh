#ifndef MODEL_HH_
#define MODEL_HH_

namespace mets {

  /// @brief Type of the objective/cost function.
  ///
  /// You should be able to change this to "int" for your uses
  /// to improve performance if it suffice, no guarantee.
  ///
  typedef double gol_type;

  /// @brief Exception risen when some algorithm has no more moves to
  /// make.
  class no_moves_error 
    : public std::runtime_error
  {
  public:
    no_moves_error() 
      : std::runtime_error("There are no more available moves.") {}
    no_moves_error(const std::string message) 
      : std::runtime_error(message) {}
  };

  /// @brief A sequence function object useful as an STL generator.
  ///
  /// Returns start, start+1, ... 
  ///
  class sequence
  {
  public:
    /// @brief A sequence class starting at start.
    sequence(int start = 0) 
      : value_m(start) 
    { }
    /// @brief Subscript operator. Each call increments the value of
    /// the sequence.
    int operator()() 
    { return value_m++; }
  protected:
    int value_m;
  };

  /// @defgroup model Model
  /// @{

  /// @brief interface of a feasible solution space to be searched
  /// with tabu search.
  ///
  /// Note that "feasible" is not intended w.r.t. the constraint of
  /// the problem but only regarding the space we want the local
  /// search to explore. From time to time allowing "feasible"
  /// solutions to move in a portion of space that is not feasible in
  /// a stricter sense is non only allowed, but encouraged to improve
  /// tabu search performances. In those cases the objective function
  /// should account for unfeasibility with a penalty term.
  class feasible_solution
  {
  public:

    /// @brief Virtual dtor.
    virtual
    ~feasible_solution() 
    { }

    /// @brief Cost function to be minimized.
    ///
    /// The cost function is the target that the search algorithm
    /// tries to minimize.
    ///
    /// You must implement this for your problem.
    ///
    virtual gol_type 
    cost_function() const = 0;

    /// @brief Assignment method.
    ///
    /// Needed to save the best solution so far.
    ///
    /// You must implement this for your problem.
    ///
    virtual void
    copy_from(const feasible_solution& other) = 0;

    feasible_solution& 
    operator=(const feasible_solution& other) 
    { this->copy_from(other); return *this; }
  };


  /// @brief An abstract permutation problem.
  ///
  /// The permutation problem provides a skeleton to rapidly prototype
  /// permutation problems (such as Assignment problem, Quadratic AP,
  /// TSP, and so on). The skeleton holds a pi_m variable that, at
  /// each step, contains a permutation of the numbers in [0..n-1].
  ///
  /// A few operators are provided to randomly choose a solution, to
  /// perturbate a solution with some random swaps and to simply swap
  /// two items in the list.
  ///
  /// You still need to provide your own
  /// mets::feasible_solution::cost_function() implementation.
  ///
  /// @see mets::swap_elements
  class permutation_problem: public feasible_solution 
  {
  public:
    
    /// @brief Unimplemented.
    permutation_problem(); 

    /// @brief Inizialize pi_m = {0, 1, 2, ..., n-1}.
    permutation_problem(int n) : pi_m(n)
    { std::generate(pi_m.begin(), pi_m.end(), sequence(0)); }

    /// @brief Copy from another permutation problem, if you introduce
    /// new member variables remember to override this and to call
    /// permutation_problem::copy_from in the overriding code.
    ///
    /// @param other the problem to copy from
    void copy_from(const feasible_solution& other);

    /// @brief The size of the problem
    virtual size_t 
    size() 
    { return pi_m.size(); }

    /// @brief: swap move
    ///
    /// Override this for delta evaluation of the cost function.
    virtual void
    swap(int i, int j)
    { std::swap(pi_m[i], pi_m[j]); }
    
  protected:
    std::vector<int> pi_m;

    template<typename random_generator> 
    friend void random_shuffle(permutation_problem& p, random_generator& rng);
  };


  /// @brief Shuffle a permutation problem (generates a random starting point).
  ///
  /// @see mets::permutation_problem
  template<typename random_generator>
  void random_shuffle(permutation_problem& p, random_generator& rng)
  {
#if defined (HAVE_UNORDERED_MAP) && !defined (TR1_MIXED_NAMESPACE)
    std::uniform_int<> unigen(0, p.pi_m.size());
    std::variate_generator<random_generator, 
      std::uniform_int<> >gen(rng, unigen);
#else
    std::tr1::uniform_int<> unigen(0, p.pi_m.size());
    std::tr1::variate_generator<random_generator, 
      std::tr1::uniform_int<> >gen(rng, unigen);
#endif
    std::random_shuffle(p.pi_m.begin(), p.pi_m.end(), gen);
  }
  
  /// @brief Perturbate a problem with n swap moves.
  ///
  /// @see mets::permutation_problem
  template<typename random_generator>
  void perturbate(permutation_problem& p, unsigned int n, random_generator& rng)
  {
#if defined (HAVE_UNORDERED_MAP) && !defined (TR1_MIXED_NAMESPACE)
    std::uniform_int<> int_range;
#else
    std::tr1::uniform_int<> int_range;
#endif
    for(unsigned int ii=0; ii!=n;++ii) 
      {
	int p1 = int_range(rng, p.size());
	int p2 = int_range(rng, p.size());
	while(p1 == p2) 
	  p2 = int_range(rng, p.size());
	p.swap(p1, p2);
      }
  }
    
  /// @brief Move to be operated on a feasible solution.
  ///
  /// You must implement this (one or more types are allowed) for your
  /// problem.
  ///
  /// You must provide an apply as well as an evaluate method.
  ///
  /// NOTE: this interface changed from 0.4.x to 0.5.x. The change was
  /// needed to provide a more general interface.
  class move
  {
  public:

    virtual 
    ~move() 
    { }; 

    ///
    /// @brief Operates this move on sol.
    ///
    /// This should actually change the solution.
    virtual void
    apply(feasible_solution& sol) const = 0;

    ///
    /// @brief Evaluate the cost after the move.
    ///
    /// What if we make this move? Local searches can be speed up by a
    /// substantial amount if we are able to efficiently evaluate the
    /// cost of the neighboring solutions without actually changing
    /// the solution.
    virtual gol_type
    evaluate(const feasible_solution& sol) const = 0;

    /// @brief Method usefull for tracing purposes.
    virtual void print(ostream& os) const { };
  };

  /// @brief A Mana Move is a move that can be automatically made tabu
  /// by the mets::simple_tabu_list.
  ///
  /// If you implement this class you can use the
  /// mets::simple_tabu_list as a ready to use tabu list, but you must
  /// implement a clone() method, provide an hash funciton and provide
  /// a operator==() method that is responsible to find if a move is
  /// equal to another.
  ///
  /// If the desired behaviour is to declare tabu the opposite of the
  /// last made move you should also override the opposite_of()
  /// method.
  ///
  class mana_move : public move
  {
  public:
    /// @brief Make a copy of this move.
    virtual mana_move* 
    clone() const = 0;
    
    /// @brief Create and return a new move that is the reverse of
    /// this one
    ///
    /// By default this just calls "clone". If this method is not
    /// overridden the mets::simple_tabu_list declares tabu the last
    /// made move. Reimplementing this method it is possibile to
    /// actually declare as tabu the opposite of the last made move
    /// (if we moved a to b we can declare tabu moving b to a).
    virtual mana_move*
    opposite_of() const 
    { return clone(); }

    /// @brief Tell if this move equals another w.r.t. the tabu list
    /// management (for mets::simple_tabu_list)
    virtual bool 
    operator==(const mana_move& other) const = 0;
    
    /// @brief Hash signature of this move (used by mets::simple_tabu_list)
    virtual size_t
    hash() const = 0;
  };


  template<typename rngden> class swap_neighborhood; // fw decl

  /// @brief A mets::mana_move that swaps two elements in a
  /// mets::permutation_problem.
  ///
  /// Each instance swaps two specific objects.
  ///
  /// @see mets::permutation_problem, mets::mana_move
  ///
  class swap_elements : public mets::mana_move 
  {
  public:  

    /// @brief A move that swaps from and to.
    swap_elements(int from, int to) 
      : p1(std::min(from,to)), p2(std::max(from,to)) 
    { }
    
    /// @brief Virtual method that applies the move on a point
    void
    apply(mets::feasible_solution& s)
    { permutation_problem& sol = reinterpret_cast<permutation_problem&>(s);
      sol.swap(p1, p2); }
            
    /// @brief An hash function used by the tabu list (the hash value is
    /// used to insert the move in an hash set).
    size_t
    hash() const
    { return (p1)<<16^(p2); }
    
    /// @brief Comparison operator used to tell if this move is equal to
    /// a move in the tabu list.
    bool 
    operator==(const mets::mana_move& o) const;
    
    void change(int from, int to)
    { p1 = std::min(from,to); p2 = std::max(from,to); }

  protected:
    int p1; ///< the first element to swap
    int p2; ///< the second element to swap

    template <typename> 
    friend class swap_neighborhood;
  };

  /// @brief A mets::mana_move that swaps a subsequence of elements in
  /// a mets::permutation_problem.
  ///
  /// @see mets::permutation_problem, mets::mana_move
  ///
  class invert_subsequence : public mets::mana_move 
  {
  public:  

    /// @brief A move that swaps from and to.
    invert_subsequence(int from, int to) 
      : p1(from), p2(to) 
    { }
    
    /// @brief Virtual method that applies the move on a point
    void
    apply(mets::feasible_solution& s);
        
    /// @brief An hash function used by the tabu list (the hash value is
    /// used to insert the move in an hash set).
    size_t
    hash() const
    { return (p1)<<16^(p2); }
    
    /// @brief Comparison operator used to tell if this move is equal to
    /// a move in the tabu list.
    bool 
    operator==(const mets::mana_move& o) const;
    
    void change(int from, int to)
    { p1 = from; p2 = to; }

  protected:
    int p1; ///< the first element to swap
    int p2; ///< the second element to swap

    // template <typename> 
    // friend class invert_full_neighborhood;
  };

  /// @brief Neighborhood generator.
  ///
  /// The move manager can represent both Variable and Constant
  /// Neighborhoods.
  ///
  /// To make a constant neighborhood put moves in the moves_m queue
  /// in the constructor and implement an empty refresh() method.
  ///
  /// To make a variable neighborhood (or to selectively select
  /// feasible moves at each iteration) update the moves_m queue in
  /// the refresh() method.
  class move_manager
  {
  public:
    ///
    /// @brief Initialize the move manager with an empty list of moves
    move_manager() 
      : moves_m() 
    { }

    /// @brief Virtual destructor
    virtual 
    ~move_manager() 
    { }

    /// @brief Iterator type to iterate over moves of the neighborhood
    typedef std::deque<move*>::iterator iterator;
    
    /// @brief Size type
    typedef std::deque<move*>::size_type size_type;

    /// @brief Begin iterator of available moves queue.
    iterator begin() 
    { return moves_m.begin(); }

    /// @brief End iterator of available moves queue.
    iterator end() 
    { return moves_m.end(); }

    /// @brief Size of the neighborhood.
    size_type size() const 
    { return moves_m.size(); }

  protected:
    std::deque<move*> moves_m; ///< The moves queue
    
  private:
    move_manager(const move_manager&);
  };
  

  /// @brief Generates a stochastic subset of the neighborhood.
#if defined (HAVE_UNORDERED_MAP) && !defined (TR1_MIXED_NAMESPACE)
  template<typename random_generator = std::minstd_rand0>
#else
  template<typename random_generator = std::tr1::minstd_rand0>
#endif
  class swap_neighborhood : public mets::move_manager
  {
  public:
    /// @brief A neighborhood exploration strategy for mets::swap_elements.
    ///
    /// This strategy selects *moves* random swaps 
    ///
    /// @param r a random number generator (e.g. an instance of
    /// std::tr1::minstd_rand0 or std::tr1::mt19936)
    ///
    /// @param moves the number of swaps to add to the exploration
    ///
    swap_neighborhood(random_generator& r, 
		      unsigned int moves);

    /// @brief Dtor.
    ~swap_neighborhood();

    /// @brief Selects a different set of moves at each iteration.
    void refresh(mets::feasible_solution& s);
    
  protected:
    random_generator& rng;
#if defined (HAVE_UNORDERED_MAP) && !defined (TR1_MIXED_NAMESPACE)
    std::uniform_int<> int_range;
#else
    std::tr1::uniform_int<> int_range;
#endif
    unsigned int n;
    unsigned int nc;

    void randomize_move(swap_elements& m, unsigned int size);
  };

  /*
  /// @brief Generates a the full swap neighborhood.
  class swap_full_neighborhood : public mets::move_manager
  {
  public:
    /// @brief A neighborhood exploration strategy for mets::swap_elements.
    ///
    /// This strategy selects *moves* random swaps.
    ///
    /// @param size the size of the problem
    swap_full_neighborhood(int size) : move_manager()
    {
      for(int ii(0); ii!=size-1; ++ii)
	for(int jj(ii+1); jj!=size; ++jj)
	  moves_m.push_back(new swap_elements(ii,jj));
    } 

    /// @brief Dtor.
    ~swap_full_neighborhood() { 
      for(std::deque<move*>::iterator it = moves_m.begin(); 
	  it != moves_m.end(); ++it)
	delete *it;
    }

    /// @brief Selects a different set of moves at each iteration.
    void refresh(mets::feasible_solution& s) { }

  };
  */

  /*
  /// @brief Generates a the full subsequence inversion neighborhood.
  class invert_full_neighborhood : public mets::move_manager
  {
  public:
    invert_full_neighborhood(int size) : move_manager()
    {
      for(int ii(0); ii!=size; ++ii)
	for(int jj(0); jj!=size; ++jj)
	  if(ii != jj)
	    moves_m.push_back(new invert_subsequence(ii,jj));
    } 

    /// @brief Dtor.
    ~invert_full_neighborhood() { 
      for(std::deque<move*>::iterator it = moves_m.begin(); 
	  it != moves_m.end(); ++it)
	delete *it;
    }

    /// @brief Selects a different set of moves at each iteration.
    void refresh(mets::feasible_solution& s) { }

  };
  */
  /// @}

  /// @brief Functor class to allow hash_set of moves (used by tabu list)
  class mana_move_hash 
  {
  public:
    size_t operator()(mana_move const* mov) const 
    {return mov->hash();}
  };
  
  /// @brief Functor class to allow hash_set of moves (used by tabu list)
  template<typename Tp>
  struct dereferenced_equal_to 
  {
    bool operator()(const Tp l, 
		    const Tp r) const 
    { return l->operator==(*r); }
  };
}
#endif
