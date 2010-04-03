#ifndef METS_ABSTRACT_SEARCH_HH_
#define METS_ABSTRACT_SEARCH_HH_

namespace mets {

  /// @defgroup common Common pieces
  /// @{

  /// @brief The solution recorder is used by search algorithm, at the
  /// end of each iteration, to record the best seen solution.
  /// 
  /// The concept of best is externalized so that you can record the
  /// best ever solution met or the best solution that matches some
  /// other criteria (e.g. feasibility constraints relaxed in the
  /// feasible_solution implementation of the cost function).
  ///
  class solution_recorder {
  public:
    /// @brief Default ctor.
    solution_recorder() {}
    /// @brief Unimplemented copy ctor.
    solution_recorder(const solution_recorder&);
    /// @brief Unimplemented assignment operator.
    solution_recorder& operator=(const solution_recorder&);

    /// @brief A virtual dtor.
    virtual 
    ~solution_recorder();

    /// @brief Accept is called at the end of each iteration for an
    /// opportunity to record the best move ever.
    ///
    /// (this is a chain of responsibility)
    ///
    virtual bool 
    accept(feasible_solution& sol) = 0;
  };

  /// @brief An abstract search.
  ///
  /// @see mets::tabu_search, mets::simulated_annealing, mets::local_search
  template<typename move_manager_type>
  class abstract_search : public subject< abstract_search<move_manager_type> >
  {
  public:
    /// @brief Set some common values needed for neighborhood based
    /// metaheuristics.
    ///
    /// @param working The working solution (this will be modified
    /// during search) 
    ///
    /// @param recorder A solution recorder instance used to record
    /// the best solution found
    ///
    /// @param moveman A problem specific implementation of the
    /// move_manager_type used to generate the neighborhood.
    ///
    abstract_search(feasible_solution& working,
		    solution_recorder& recorder,
		    move_manager_type& moveman)
      : subject<abstract_search<move_manager_type> >(), 
	solution_recorder_m(recorder),
	working_solution_m(working),
	moves_m(moveman),
	current_move_m(),
	step_m()
    { }
			 
    /// purposely not implemented (see Effective C++)
    abstract_search(const abstract_search<move_manager_type>&);
    /// purposely not implemented (see Effective C++)
    abstract_search& operator==(const abstract_search<move_manager_type>&);

    /// @brief Virtual destructor.
    virtual 
    ~abstract_search() 
    { };

    /// @brief We just made a move.
    static const int MOVE_MADE = 0;
    /// @brief Our solution_recorder_chain object reported an improvement
    static const int IMPROVEMENT_MADE = 0;

    /// @brief This method starts the search.
    /// 
    /// Remember that this is a minimization.
    ///
    /// An exception mets::no_moves_error can be risen when no move is
    /// possible.
    virtual void
    search() 
      throw(no_moves_error) = 0;

    /// @brief The solution recorder instance.
    const solution_recorder&
    get_solution_recorder() const 
    { return solution_recorder_m; };

    /// @brief The current working solution.
    const feasible_solution&
    working() const 
    { return working_solution_m; }
    
    feasible_solution&
    working() 
    { return working_solution_m; }

    /// @brief The last move made
    const move&
    current_move() const 
    { return **current_move_m; }

    /// @brief The last move made
    move&
    current_move() 
    { return **current_move_m; }
 
    /// @brief The move manager used by this search
    const move_manager_type& 
    move_manager() const 
    { return moves_m; }

    /// @brief The move manager used by this search
    move_manager_type& 
    move_manager() 
    { return moves_m; }

    /// @brief The current step of the algorithm (to be used by the
    ///        observers).
    ///        
    /// When you implement a new type of search you should set step_m
    /// protected variable to the status of the algorithm
    /// (0 = "MOVE_MADE", 1 = "IMPROVEMENT_MADE", etc.).
    int
    step() const 
    { return step_m; }
    
  protected:
    solution_recorder& solution_recorder_m;
    feasible_solution& working_solution_m;
    move_manager_type& moves_m;
    typename move_manager_type::iterator current_move_m;
    int step_m;
  };

  /// @}

  /// @defgroup common Common pieces
  /// @{

  /// @brief The best ever solution recorder can be used as a simple
  /// solution recorder that just records the best copyable solution
  /// found during its lifetime.
  /// 
  class best_ever_solution : public solution_recorder 
  {
  public:
    best_ever_solution(copyable_solution& best) : 
      solution_recorder(), 
      best_ever_m(best) 
    { }

    /// @brief Unimplemented default ctor.
    best_ever_solution();
    /// @brief Unimplemented copy ctor.
    best_ever_solution(const best_ever_solution&);
    /// @brief Unimplemented assignment operator.
    best_ever_solution& operator=(const best_ever_solution&);

    /// @brief Accept is called at the end of each iteration for an
    /// opportunity to record the best move ever.
    bool accept(feasible_solution& sol);

    /// @brief Returns the best solution found since the beginning.
    const copyable_solution& best_ever() const 
    { return best_ever_m; }

  protected:
    /// @brief Records the best solution
    copyable_solution& best_ever_m;
  };
    
  /// @brief An object that is called back during the search progress.
  template<typename move_manager_type>
  class search_listener : public observer<abstract_search<move_manager_type> >
  {
  public:
    typedef abstract_search<move_manager_type> search_type;
    /// @brief A new observer (listener) of a search process, remember
    /// to attach the created object to the search process to be
    /// observed (mets::search_type::attach())
    explicit
    search_listener() : observer<search_type>() 
    { }

    /// purposely not implemented (see Effective C++)
    search_listener(const search_listener<search_type>& other);
    search_listener<search_type>& 
    operator=(const search_listener<search_type>& other);

    /// @brief Virtual destructor
    virtual 
    ~search_listener() 
    { }

    /// @brief This is the callback method called by searches
    /// when a move, an improvement or something else happens
    virtual void
    update(search_type* algorithm) = 0;
  };

  /// @}


}
#endif